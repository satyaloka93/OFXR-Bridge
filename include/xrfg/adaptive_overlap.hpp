#pragma once

#include <cstdint>

namespace xrfg {
// Uses complete application-frame intervals, not the wait call whose location
// changes between modes. All calls are serialized by the layer frame mutex.
class AdaptiveOverlap {
public:
    enum class Mode : std::uint32_t { overlap, baseline, probe };

    bool select(std::int64_t now_us, std::int64_t target_us, bool eligible) noexcept {
        if (!eligible || target_us <= 0) {
            reset();
            return false;
        }
        if (last_us_ == 0 || now_us <= last_us_ ||
            (target_us_ && (target_us > target_us_ * 105 / 100 || target_us < target_us_ * 95 / 100))) {
            reset();
            last_us_ = now_us;
            target_us_ = target_us;
            return true;
        }
        const auto interval = now_us - last_us_;
        last_us_ = now_us;
        target_us_ = target_us;
        // Loading/focus stalls are not representative performance samples.
        if (interval > 250000) {
            clear_window();
            return mode_ != Mode::baseline;
        }
        if (warmup_) {
            --warmup_;
            return mode_ != Mode::baseline;
        }
        sum_us_ += interval;
        if (++samples_ < 24) return mode_ != Mode::baseline;
        last_mean_us_ = sum_us_ / samples_;
        sum_us_ = 0;
        samples_ = 0;
        if (mode_ == Mode::baseline) {
            baseline_us_ = last_mean_us_;
            if (now_us >= retry_us_) {
                mode_ = Mode::probe;
                clear_window();
            }
        } else if (mode_ == Mode::probe) {
            // Keep a trial only if it reaches near the intended cadence or
            // improves throughput by at least 5% over the latest baseline.
            if (last_mean_us_ <= target_us * 115 / 100 ||
                (baseline_us_ && last_mean_us_ <= baseline_us_ * 95 / 100)) {
                mode_ = Mode::overlap;
            } else {
                fallback(now_us);
            }
        } else if (last_mean_us_ > target_us * 135 / 100 &&
                   (!baseline_us_ || last_mean_us_ > baseline_us_ * 110 / 100)) {
            fallback(now_us);
        }
        return mode_ != Mode::baseline;
    }

    void reset() noexcept { *this = AdaptiveOverlap{}; }
    Mode mode() const noexcept { return mode_; }
    std::int64_t mean_us() const noexcept { return last_mean_us_; }
    std::int64_t baseline_us() const noexcept { return baseline_us_; }

private:
    void clear_window() noexcept { sum_us_ = 0; samples_ = 0; warmup_ = 4; }
    void fallback(std::int64_t now_us) noexcept {
        mode_ = Mode::baseline;
        retry_us_ = now_us + 8000000;
        clear_window();
    }
    Mode mode_{Mode::overlap};
    std::int64_t last_us_{}, target_us_{}, sum_us_{}, last_mean_us_{}, baseline_us_{}, retry_us_{};
    unsigned samples_{}, warmup_{4};
};
} // namespace xrfg
