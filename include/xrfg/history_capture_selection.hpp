#pragma once

namespace xrfg {

// Kept per swapchain, so a reused runtime handle cannot inherit projection use.
// Callers serialize access. Once used, retain capture for reused-image frames
// and returning projections; only never-used resources are filtered.
class HistoryCaptureSelection {
public:
    void observe_projection() noexcept { used_as_projection_ = true; }
    [[nodiscard]] bool capture(bool selection_enabled) const noexcept {
        return !selection_enabled || used_as_projection_;
    }
private:
    bool used_as_projection_{};
};

} // namespace xrfg
