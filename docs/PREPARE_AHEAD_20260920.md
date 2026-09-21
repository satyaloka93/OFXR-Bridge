# Preparing synthesis before presenter admission

**Withdrawn from normal use September 20:** both Silent Hill f and The Outer
Worlds 2 failed to enter VR with this build. The previously working retention
build has been restored to both the installed runtime and tray source DLL.
The experiment now requires `OFXR_EXPERIMENTAL_PREPARE_AHEAD=1`; default builds
do not allocate the additional swapchains. Tests opt in explicitly.

The restoration above describes the rollback at that time. The current build
instead uses [earlier internal optical-flow preparation](EARLY_OPTICAL_FLOW_20260920.md)
without extra runtime swapchains and has since run in both games.

Failure logs are archived in `build/injection-failure-20260920`. OpenXR instance,
session and three synthesis contexts initialize successfully, but no application
frame loop starts. TOW2's UEVR log explicitly reports swapchain creation failure
after those contexts. Three application color swapchains plus four private
swapchains each and one static swapchain account for 16 created swapchains;
the next application creation fails. A runtime count/resource limit is the
leading explanation, not yet a proven error code. Allocation must be deferred
until application setup is complete, with safe fallback, before reconsidering
this experiment. The fake runtime did not model this aggregate startup limit.

The September 20 11:01 run spent about 9.7–10.0 ms per application frame
waiting for presenter capacity with Cheeky off, versus 5.9 ms on. This gate
preceded synthesis submission, serializing work that can use independent images.

SteamVR D3D12 generation now attempts to allocate a second synthetic swapchain.
It alternates with the existing synthetic swapchain using the same slot as the
two existing current-image swapchains. Destination indices include the synthetic
slot stride; the synthesizer's existing destination fences still protect reuse.
If creating/enumerating the extra swapchain fails, the single-output path remains.
D3D11 and other runtimes retain their existing output allocation.

For a continuous, non-pipelined presenter whose mapped resources all have both
outputs, preparation may start with at most two outstanding submissions. FIFO
retirement means the pair two frames back has finished, so the selected output
set is no longer referenced by a pending submission. The application still waits
for at most one outstanding submission **after** preparation and before handoff.
Thus the maximum queued work at handoff is unchanged; the change overlaps GPU
preparation with the existing presentation wait rather than removing backpressure.
Pipelined callers and single-output resources retain the pre-preparation gate.

The synthesis-completion gate at image capture/end-frame, private acquire/wait/
release tracking, destruction drain, and single in-flight synthesis limit remain.
Both synthetic swapchains are released and destroyed during normal cleanup.

This costs one extra runtime swapchain per eligible SteamVR D3D12 application
swapchain. Three 6800×3468 four-byte images have about 270 MiB of pixel storage,
before allocation overhead; smaller eligible swapchains also allocate another
output. Actual residency and in-game frame-time improvement must be measured.

The delayed-presenter layer test holds runtime consumption for 400 ms and checks
that the following pair releases its outputs during that hold, while neither
held output's release version changes. A companion test rejects the additional
swapchain allocation and requires the original blocking behavior. Existing tests
exercise reused images, disarm, source changes, private lifetime and pipelining.

Release build, all 47 CTest cases, and NVIDIA hardware tests passed. After adding
a bounded watchdog to the new test, both preparation/fallback cases passed again.
The preceding DLL, copied beside the same test INI, fails the new case with
`Prepare ahead=0 held synthetic unchanged=1`; the new DLL passes it.

Installed into `RuntimeLayer/v116` at 11:21:38 September 20. The preceding DLL
was backed up with suffix `.before-prepare-ahead-20260920-112138.bak`. Built and
installed SHA256 match:
`F0EF85130FB271C2281B705B6B577C7248616591C9550F6544C15A0778D82DDB`.
