# Bounded game-frame overlap

The Silent Hill f run at 2026-09-19 22:21:32 spent 11.75 ms (Cheeky off) and
8.48 ms (on) waiting before synthesis GPU execution, versus 3.83/3.84 ms executing
synthesis. The old frame-start fence gate prevented the CPU from preparing the
next game frame until the previous synthesis transaction completed.

This change defers that gate for the continuous, non-pipelined presenter path.
The next game frame can perform CPU work and enqueue commands behind the existing
synthesis job. Before its image enters OFXR history, swapchain release waits for
the previous synthesis fence. End-frame repeats this check for frames that reuse
the previous image. A single outstanding synthesis transaction remains the limit.
No fences are removed, no history slots are shared concurrently, and no private
output is overwritten while pending. Timeout behavior and nonblocking submission
rejection are retained. This is CPU/GPU overlap, not a new asynchronous GPU queue.
It does not guarantee a reduction in the measured queue delay or a FPS gain.

Inline runtime pacing and pipelined applications retain the original gate.
The `synthesis_admission` flight record identifies whether this path is active.
The process-lifetime crash fix remains included.

## Verification

Two layer-level WARP tests deliberately block synthesis with an unsignaled GPU
fence for 400 ms. They require wait-frame admission in under 150 ms while the
image-release/end-frame boundary blocks at least 150 ms. Both new-image and
reused-image paths are covered. The previous installed DLL fails the new-image
test with admission=410 ms and protected boundary=0 ms. The new implementation
passes both. Existing tests cover history backpressure, explicit destruction,
pipelined handoffs, presenter lifetime, manual disarm and graphics interoperability.

For the in-game comparison, use the same scene and off/on ordering as the last
run. Keep flight logging enabled. Compare application FPS, p95/p99 frame intervals,
queue delay, synthetic/new/repeated submission counts, and errors. A shorter
xrWaitFrame alone is not success: the wait can simply have moved to image release.

## Adaptive fallback

The unconditional overlap experiment regressed the 22:35:42 Silent Hill f run
with Cheeky off (33.2 to 22.5 FPS), but improved it with Cheeky on (34.5 to 44.3).
Overlap now uses an application-frame-interval controller, independent of image
capture and independent of Cheeky's internal toggle state.

- Observe complete intervals between application wait-frame admissions, including
  the intervening rendering, release and end-frame work. Ignore gaps over 250 ms.
- Discard four settling intervals after a mode change; evaluate 24-frame means.
- Fall back to the original frame-start gate when overlap exceeds 135% of the
  target two-display-period cadence and is more than 10% slower than a known
  baseline (or no baseline has been measured yet).
- Measure baseline continuously. After at least eight seconds, try overlap again
  at a completed measurement window. Keep it only if it reaches within 15% of
  target cadence or beats the latest baseline by at least 5%.
- A failed probe returns to baseline for another cooldown. This intentionally
  permits brief slower probe windows so a workload improvement can be detected.
  It cannot promise zero transient regression, nor identify a Cheeky toggle directly.
- Reset on ineligible/pipelined paths, session reset or a display-period change.

`synthesis_overlap_policy` logs transitions: result=0 retained overlap,
1 baseline, 2 trial overlap; a=last measured interval mean (microseconds),
b=baseline mean, c=target interval. `synthesis_admission` still logs the actual
selected gate. Fence, history and output ownership checks are unchanged.

Unit tests cover fallback using the recorded 44.51 ms cadence, cooldown,
rejected probes, recovery using 22.556 ms cadence, a return to heavy load,
loading stalls, jitter, ineligible paths and refresh-rate changes. The existing
GPU-blocked capture/reused-image tests cover the actual fence boundaries.
