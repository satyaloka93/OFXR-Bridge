# Explicit frame-scheduling comparison

`ofxr-scheduling.ini` beside the loaded DLL selects the schedule at session
creation. It is separate from the tray-generated configuration:

```ini
[scheduling]
mode=3
```

Modes: 0 adaptive (default), 1 original frame-start fence wait, 2 overlap with
the fence wait at image release/end-frame, 3 timed comparison. Invalid values
use adaptive. Every override respects the existing continuous/non-pipelined,
armed-session eligibility checks. Buffers, fence ownership and queue limits are
unchanged. The extra-swapchain experiment stays disabled by default.

Comparison starts when overlap first becomes eligible. It uses 30 seconds of
adaptive warm-up, then 30 seconds each of original, overlap, original, overlap.
After 150 seconds it returns to adaptive for the rest of that session. A new
session starts a new comparison until configuration is restored to mode 0.
Keep the scene, DLSS quality, Cheeky state and headset refresh rate fixed.
Discard the first five seconds of each measured block. Compare both repetitions
and frame-time tails, not just FPS. A loading/menu interruption invalidates the
affected block; this is not a guarantee that two scenes have identical GPU work.

Use `scripts/Set-OFXRScheduling.ps1 -Mode Compare` before starting the game.
Restore normal behavior with `-Mode Adaptive`. Original and Overlap can also be
selected explicitly for a whole session. Restart the game to apply a change.

Flight event `scheduling_experiment`: result=selected schedule (0/1/2), a=requested
mode, b=phase (0 warm-up, 1–4 comparison blocks, 5 finished), c=eligible. Events
mark phase/schedule changes. Adaptive history resets on those transitions.
`synthesis_admission` continues to report the effective fence placement.

Run `python scripts/analyze-scheduling.py <original-flight.log>` to combine the
numbered segments and report complete labelled blocks. It excludes settling and
interrupted/incomplete blocks. Reported waits are not additive with frame times.

Verification: all 50 tests pass. A 400 ms GPU block produces a 404 ms admission
wait and 0 ms protected-boundary wait in forced-original mode; forced-overlap
produces 0 ms admission and 405 ms protected-boundary wait. Clock-driven tests
cover all comparison phases, return to adaptive, invalid configuration and
ineligible paths. Analyzer verification covers segmented input, settling,
known 50 FPS cadence and incomplete-block exclusion.

Also removed an erroneous completion event inside the presenter wait predicate.
Only the actual return from the wait now emits its completion record. This fixes
duplicate/premature diagnostic results without changing that wait's behavior.
