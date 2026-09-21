# Bridge flight recorder

The recorder belongs to the OFXR OpenXR layer and is independent from game or
VR-mod logging. It is disabled by default.

V066 also provides an independent transparent numeric FPS overlay. It is green
when a recent synthetic submission succeeded and red when generation is
inactive. It counts accepted nonempty OpenXR submissions, not physical headset
scanout, and does not replace the recorder for crash or fallback diagnosis. See
[FPS_OVERLAY.md](FPS_OVERLAY.md) for its controls and limitations.

Enable **Bridge flight recorder** from the tray before starting the game. Each
OpenXR process then creates a file named
`ofxr-bridge-flight-YYYYMMDD-HHMMSS-pidNNNN.log`. Use **Open bridge logs** to
open its directory.

The log records layer negotiation, graphics binding, swapchain eligibility,
projection mapping, frame-generation stages, recovery events and potentially
blocked OpenXR call boundaries. An unmatched begin record can identify the
operation which did not return during a freeze.

For a compatibility report, reproduce the problem once and attach the newest
flight log and all its `.partN.log` segments. Also provide the game and version, VR mod, headset, OpenXR runtime,
GPU and driver, selected backend/options and exact reproduction steps. If no
log was created, state that explicitly: the layer may not have loaded or the
process may have stopped before recorder initialization.

The recorder does not capture video and is not a replacement for a native
crash dump. Game logs and crash dumps remain useful when available.

## Fidelity captures (local diagnostic build)

Enable the flight recorder. Before starting Steam/game, set the environment
variable `OFXR_FIDELITY_CAPTURE=1` in the process that launches them (a running
Steam process will not inherit a newly set variable). Press and release **F8**
during gameplay to capture the next new synthesis pair. Up to four pairs per
synthesizer are allowed per session; restart the game to reset this limit.
Capture is disabled unless both the environment opt-in and flight recorder are on.

A folder named after the flight log with `-captures` appended receives folders
named `synthesizer-address-previousSerial-currentSerial`. Each contains the
previous/current color inputs, synthetic output, and available current DLSS
motion/depth textures. `pair.txt` records capture IDs, the route actually used,
view rectangles, poses/FOV, and DLSS stream/epoch/publication/jitter information.
Raw texture files preserve GPU bytes, with `.raw.json` sidecars describing DXGI
format and each mip/array subresource's offset, row pitch and dimensions.
Use `python scripts/preview-fidelity-capture.py path/to/current.raw` to produce
PPM previews of supported color formats. Motion/depth data remain raw.

Copies are recorded on the synthesis queue and mapped only after the work-slot
fence completes on reuse. No new CPU fence wait is added. GPU copies and disk
writes still perturb timing: do not use capture frames for performance comparisons.
Each texture is capped at 128 MiB. Multisampled/multiplanar textures are skipped.
A capture immediately before shutdown may not be flushed. Disk/allocation failures
are best-effort and may leave partial captures. Captures do not prove physical
headset scanout or that a producer's published IDs are truthful.

Additional flight records:

- `fidelity_input`: result=pair requested; a=application swapchain, b=capture serial,
  c=source image index. Serials are scoped to a swapchain.
- `fidelity_motion`: result=eye; a=DLSS stream, b=serial, c=publication. These follow
  their input record on the same application thread; absence means no resolved guide.
- `fidelity_present`: result bit flags: 1=new queued request, 2=synthetic owned
  frame, 4=nonempty submission, 8=failed submission. a=original owned-frame display
  time (zero for borrowed/unavailable), b=runtime display time, c=request sequence
  (zero for repeat/empty). This complements the existing submission result record.

## Queue performance counters

Normal flight logging now includes `nvidia_queue_delay`: result=0 means clock
calibration succeeded, result=1 means unavailable; a=microseconds from the queue
clock calibration immediately before pack submission to the GPU pack-begin
timestamp, b=GPU synthesis duration, c=current capture serial. This is an
approximate submission-to-start interval (including submission overhead), not a
pure measure of game GPU time. Calibration is only queried when GPU timing is
already enabled. `dlss_guide_status` is sampled every 120 capture serials:
result=guide status enum, a=published, b=matched, c=used. Selecting DLSS motion
vectors does not itself publish guides; a cooperating producer must call OFXR's
provider API. No image captures or F8 presses are needed for these counters.

## Bounded game-frame overlap

For an active continuous presenter with a non-pipelined application, the layer
admits one next application frame before waiting for the prior synthesis fence.
The same bounded fence wait occurs at the next eligible swapchain image release,
before history capture, and at end-frame for reused images. Inline rendering and
pipelined application frame calls retain the original frame-start gate.
Submission still rejects a pending synthesis transaction, and history leases,
private destination availability and presenter ownership remain checked.

`synthesis_admission` logs changes: result=1 selects overlap, result=0 returns to
the original gate; a=session. The existing `synthesis_frame_start_wait` counter
keeps its historical name, but in overlap mode it measures a wait nested inside
swapchain release/end-frame. Compare total application frame rate and frame-time
tails, not just xrWaitFrame duration: moving a wait alone is not a measured gain.
No F8 capture is needed. The previous runtime DLL is the rollback/A-B baseline.

The overlap path is now adaptive. `synthesis_overlap_policy` result values are
0=retained overlap, 1=original gate, 2=trial overlap; a/b/c report measured,
baseline and target application intervals in microseconds. It retries after an
eight-second baseline cooldown plus the current measurement window, so allow
roughly ten seconds after changing Cheeky for adaptation. Short trial slowdowns
can occur when the original gate is better. See
[the policy and verification notes](BOUNDED_GAME_FRAME_OVERLAP.md).

## Selective history capture

`history_capture_skipped` records an eligible image release whose history copy
was skipped because its swapchain has never been used as a projection source.
`a` is the application swapchain handle, `b` is the released image index, and
`result`/`c` are zero. This is a count of avoided capture attempts, not measured
GPU savings. Selection starts after a valid projection mapping in non-pipelined
D3D12 sessions. Newly selected sources recover through fresh capture and priming;
skipped releases invalidate old capture tickets.
## Recorder segments and presenter waits

At `max_file_mb` (default 32 MiB), the recorder continues in
`<original-name>.part1.log`, `.part2.log`, and so on, preserving the original
stem and replacing its `.log` extension. Keep all files from the run together.
Sequence numbers and elapsed timestamps continue across segments; a begin/end
pair can span files. `log_path()` continues to identify the first file.
The limit now applies per segment, not to total storage for a run. If creating
a segment fails, the recorder keeps appending to the current file rather than
discarding data, and does not retry creation on every frame.

`presenter_capacity_wait` measures a wait for the presenter queue to reach the
allowed outstanding count: a=session, b=allowed count, c=actual outstanding
count at the boundary. `presenter_completion_wait` measures waiting for a
particular submission: a=session, b=request sequence, c=completed at end.
Both emit begin/end pairs only when their initial predicate requires a wait;
the end result reports success, shutdown or presenter failure. Their intervals
are nested inside application calls and must not be added to those call times.

`nvidia_output_admission` records admission after internal NVIDIA preparation
and before output-writing commands. GPU total timestamps can include this
admission gap; see [the timing and validation notes](EARLY_OPTICAL_FLOW_20260920.md)
before interpreting them as execution cost.
