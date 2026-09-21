#include "xrfg/adaptive_overlap.hpp"
#include "xrfg/scheduling_experiment.hpp"
#include <iostream>
#include <stdexcept>

using Policy = xrfg::AdaptiveOverlap;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Run {
    Policy policy;
    std::int64_t now{1000000};
    bool step(std::int64_t interval = 22222, bool eligible = true, std::int64_t target = 22222) {
        now += interval;
        return policy.select(now, target, eligible);
    }
    void frames(unsigned count, std::int64_t interval) { while (count--) step(interval); }
};
int main() {
    try {
        xrfg::SchedulingExperiment comparison(3);
        require(comparison.select(1000000, false) == 0, "ineligible comparison enabled");
        require(comparison.select(2000000, true) == 0, "comparison omitted warmup");
        require(comparison.select(31999999, true) == 0, "warmup ended early");
        require(comparison.select(32000000, true) == 1, "first original block missing");
        require(comparison.select(62000000, true) == 2, "first overlap block missing");
        require(comparison.select(63000000, false) == 0, "comparison bypassed eligibility");
        require(comparison.select(92000000, true) == 1, "second original block missing");
        require(comparison.select(122000000, true) == 2, "second overlap block missing");
        require(comparison.select(152000000, true) == 0 && comparison.phase() == 5,
            "comparison did not restore adaptive scheduling");
        require(comparison.select(252000000, true) == 0, "finished comparison restarted");
        xrfg::SchedulingExperiment forced(2), invalid(99);
        require(forced.select(1, true) == 2 && forced.select(2, false) == 0,
            "forced overlap ignored safety eligibility");
        require(invalid.select(1, true) == 0, "invalid setting did not use adaptive");
        Run r;
        require(r.step(), "initial overlap not enabled");
        r.frames(28, 44510); // measured Cheeky-off overlap cadence
        require(r.policy.mode() == Policy::Mode::baseline, "slow overlap did not fall back");
        r.frames(200, 30150); // measured previous Cheeky-off cadence
        require(r.policy.mode() == Policy::Mode::baseline, "retry happened before cooldown");
        for (unsigned i = 0; i < 100 && r.policy.mode() == Policy::Mode::baseline; ++i) r.step(30150);
        require(r.policy.mode() == Policy::Mode::probe, "baseline never retried overlap");
        r.frames(28, 44510);
        require(r.policy.mode() == Policy::Mode::baseline, "losing probe was retained");
        for (unsigned i = 0; i < 400 && r.policy.mode() == Policy::Mode::baseline; ++i) r.step(28950);
        require(r.policy.mode() == Policy::Mode::probe, "second probe missing");
        r.frames(28, 22556); // measured Cheeky-on overlap cadence
        require(r.policy.mode() == Policy::Mode::overlap, "winning probe was rejected");
        r.frames(200, 22556);
        require(r.policy.mode() == Policy::Mode::overlap, "steady fast mode oscillated");
        r.frames(24, 44510);
        // Depending on the window phase, a second window can be needed.
        r.frames(24, 44510);
        require(r.policy.mode() == Policy::Mode::baseline, "returning heavy workload did not fall back");

        Run stalls;
        stalls.step(); stalls.step(1000000); stalls.frames(28, 22222);
        require(stalls.policy.mode() == Policy::Mode::overlap, "single loading stall caused fallback");
        require(!stalls.step(22222, false), "ineligible path overlapped");
        require(stalls.step(), "re-entry did not restart policy");
        require(stalls.step(16666, true, 16666), "refresh-rate change failed");
        require(!stalls.step(22222, true, 0), "invalid display period overlapped");
        Run jitter;
        jitter.step();
        for (unsigned i = 0; i < 200; ++i) jitter.step(i % 2 ? 27000 : 18000);
        require(jitter.policy.mode() == Policy::Mode::overlap, "normal jitter caused mode chatter");
        std::cout << "Adaptive overlap: fallback, cooldown, losing/winning probes, workload changes and reset passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
