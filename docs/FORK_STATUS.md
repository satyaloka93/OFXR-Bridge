# Performance fork status — September 20, 2026

Based on upstream v0.2.1 / V116, commit `dad56ac`. Upstream already incorporates
djules presenter contributions; see [the integration notes](PR3_INTEGRATION.md).
The separate djules V185 binary has not been compared under sufficiently
controlled conditions to claim universal superiority. No visual-fidelity
assessment is claimed. UEVR performance CVar changes are outside this work.

## Changes

- Process-lifetime registries fix an identified late-shutdown downstream call
  hazard while preserving explicit OpenXR cleanup.
- Bounded CPU/GPU overlap defers synthesis admission to protected image boundaries.
  Adaptive fallback and retry address workloads where unconditional overlap hurts.
- Selective history capture avoids copying swapchains never used for projection.
- NVIDIA packing/flow can be queued before presenter admission. Output commands
  remain protected, with cancellation/retry covered by completion fences.
- Segmented recorder retention and timing counters expose queue delay, presenter
  waits, scheduling state and output admission without losing earlier records.
- Optional fidelity capture and scheduling comparison utilities support diagnosis;
  image capture is unnecessary for performance testing.

No additional runtime swapchains are required by the current early-flow change.
The previous extra-synthetic-swapchain experiment broke injection in both games
and remains disabled unless `OFXR_EXPERIMENTAL_PREPARE_AHEAD=1` is explicitly set.
Keep it disabled for normal use. Adaptive is the normal scheduling default.

## Verification and observed results

All 50 CTest cases and NVIDIA hardware checks passed. Admission tests cover
pending-work cancellation, untouched outputs before admission, retry and reuse.

| Run | Observation | Interpretation |
|---|---|---|
| TOW2 scheduling comparison | Original 25.00 vs overlap 38.83 app FPS; p95 46.423 vs 30.752 ms | Equal 23-second windows before recorded CVar changes support overlap for this workload; scenes remain uncontrolled |
| SHf latest build, Cheeky on | Final analyzed 115 s: 44.66 app FPS, p95 22.996 ms, no >40 ms intervals | Early-flow path worked; no matched old-build baseline |
| TOW2 latest build, settled Cheeky on/off | 39.96/39.14 app FPS, p95 33.667/40.117 ms | Typical pacing favored on; both states had large hitches |

TOW2's settled sections show about 2.38 ms combined packing/flow/composition
intervals in either state, versus queue delay of 12.60/14.48 ms on/off. The
queue delay includes preceding work. The worst 558 ms application interval
contains only about 2.11 ms of recorded application OpenXR calls; the trace
locates most of that pause between calls without identifying its cause.

Latest SHf/TOW2 logs have no missing sequence IDs or negative OFXR results.
They lack explicit session/instance destruction records, so orderly OpenXR
teardown is not established. These game tests do not prove an incremental
speedup from early flow, universal crash freedom or superiority over fixed
overlap in every workload. Presenter submissions do not measure headset scanout.

## Knowledge and reproduction

The [OKF bundle](../.okf/index.md) records implementation guarantees, validation
limits, build/deployment workflow and timing interpretation. Detailed notes:

- [Bounded overlap and adaptive policy](BOUNDED_GAME_FRAME_OVERLAP.md)
- [Scheduling comparison](SCHEDULING_COMPARISON.md)
- [Early optical flow](EARLY_OPTICAL_FLOW_20260920.md)
- [Disabled allocation experiment](PREPARE_AHEAD_20260920.md)
- [Diagnostics](DIAGNOSTICS.md)

Raw game logs and machine-specific deployment backups remain local in ignored
build directories; no binaries or raw logs are included in the source commit.
