#pragma once
#include <cstdint>

namespace xrfg {
// Requested mode: 0 adaptive, 1 original, 2 overlap, 3 timed A/B/A/B.
// Called under the application's frame-call lock. Safety eligibility always
// takes precedence over an experimental override.
class SchedulingExperiment {
public:
    explicit SchedulingExperiment(unsigned mode = 0) noexcept
        : requested_(mode <= 3 ? mode : 0) {}
    unsigned select(std::int64_t now_us, bool eligible) noexcept {
        if (!eligible) return 0;
        if (requested_ != 3) return requested_;
        if (origin_us_ < 0 || now_us < origin_us_) origin_us_ = now_us;
        const auto block = (now_us - origin_us_) / 30000000;
        // Warm up for 30 s, run four 30 s blocks, then leave comparison mode.
        phase_ = block > 4 ? 5 : static_cast<unsigned>(block);
        return phase_ == 0 || phase_ == 5 ? 0 : (phase_ % 2 ? 1 : 2);
    }
    unsigned requested() const noexcept { return requested_; }
    unsigned phase() const noexcept { return phase_; }
private:
    unsigned requested_{}, phase_{};
    std::int64_t origin_us_{-1};
};
} // namespace xrfg
