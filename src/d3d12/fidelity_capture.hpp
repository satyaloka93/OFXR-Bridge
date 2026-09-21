#pragma once

// Opt-in, bounded diagnostic readback. No CPU fence wait is introduced.
#include <d3d12.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

namespace xrfg {
struct FidelityReadback {
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
    std::vector<UINT> rows;
    std::vector<UINT64> row_bytes;
    std::filesystem::path path;
    UINT64 size{};
    DXGI_FORMAT format{};
    std::string pair_metadata;
};

inline void finish_fidelity_capture(std::vector<FidelityReadback>& pending) noexcept {
    for (auto& item : pending) {
        void* data = nullptr;
        const D3D12_RANGE range{0, static_cast<SIZE_T>(item.size)};
        if (FAILED(item.buffer->Map(0, &range, &data))) continue;
        try {
            std::filesystem::create_directories(item.path.parent_path());
            if (!item.pair_metadata.empty()) {
                std::ofstream notes(item.path.parent_path() / "pair.txt");
                notes << item.pair_metadata;
            }
            std::ofstream raw(item.path, std::ios::binary);
            raw.write(static_cast<const char*>(data), static_cast<std::streamsize>(item.size));
            std::ofstream meta(item.path.string() + ".json");
            meta << "{\"format\":" << static_cast<unsigned>(item.format) << ",\"subresources\":[";
            for (size_t i = 0; i < item.layouts.size(); ++i) {
                const auto& f = item.layouts[i];
                if (i) meta << ',';
                meta << "{\"offset\":" << f.Offset << ",\"width\":" << f.Footprint.Width
                     << ",\"height\":" << f.Footprint.Height << ",\"pitch\":" << f.Footprint.RowPitch
                     << ",\"rows\":" << item.rows[i] << ",\"row_bytes\":" << item.row_bytes[i] << '}';
            }
            meta << "]}";
        } catch (...) { /* Diagnostics must not interrupt rendering. */ }
        const D3D12_RANGE no_write{0, 0};
        item.buffer->Unmap(0, &no_write);
    }
    pending.clear();
}

inline void record_fidelity_texture(ID3D12Device* device,
    ID3D12GraphicsCommandList* commands, ID3D12Resource* source,
    D3D12_RESOURCE_STATES state, const std::filesystem::path& path,
    std::vector<FidelityReadback>& pending) {
    if (!source) return;
    const auto desc = source->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.SampleDesc.Count != 1) return;
    D3D12_FEATURE_DATA_FORMAT_INFO info{desc.Format, 0};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &info, sizeof(info))) || info.PlaneCount != 1) return;
    const UINT count = desc.MipLevels * desc.DepthOrArraySize;
    FidelityReadback item;
    item.path = path;
    item.format = desc.Format;
    item.layouts.resize(count); item.rows.resize(count); item.row_bytes.resize(count);
    device->GetCopyableFootprints(&desc, 0, count, 0, item.layouts.data(),
        item.rows.data(), item.row_bytes.data(), &item.size);
    // Bound each texture to 128 MiB, and the caller to eight textures per pair.
    if (!item.size || item.size > 128ULL * 1024 * 1024) return;
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = item.size; buffer.Height = 1; buffer.DepthOrArraySize = 1;
    buffer.MipLevels = 1; buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(item.buffer.GetAddressOf())))) return;
    pending.push_back(std::move(item)); // Allocate before recording references.
    auto& saved = pending.back();
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition = {source, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state, D3D12_RESOURCE_STATE_COPY_SOURCE};
    if (state != D3D12_RESOURCE_STATE_COPY_SOURCE) commands->ResourceBarrier(1, &barrier);
    for (UINT i = 0; i < count; ++i) {
        D3D12_TEXTURE_COPY_LOCATION from{}; from.pResource = source;
        from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; from.SubresourceIndex = i;
        D3D12_TEXTURE_COPY_LOCATION to{}; to.pResource = saved.buffer.Get();
        to.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; to.PlacedFootprint = saved.layouts[i];
        commands->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
    }
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    if (state != D3D12_RESOURCE_STATE_COPY_SOURCE) commands->ResourceBarrier(1, &barrier);
}
} // namespace xrfg
