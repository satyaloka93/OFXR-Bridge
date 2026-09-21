# Silent Hill f shutdown crash and OFXR performance

## Crash evidence and fix

The 2026-09-19 19:45:57 Silent Hill f flight log (PID 33428) ends after successful
frame calls at 8.404 s and a logger shutdown record at 10.140 s. UEVR's exception
at 19:46:07.503 is on thread 18964, in CheekyOpenXRLayer.dll + 0x6563. The saved
stack contains a return address in OFXR + 0x4150f, immediately after the overlay
Impl destructor's indirect destroy_swapchain call (function pointer at +0x40).
Further frames lead through OFXR's static cleanup and CRT/DLL shutdown. This is
a downstream call during teardown, not evidence that Cheeky foveated rendering
caused the crash. The Cheeky instruction reads a hash bucket via RCX=0x1e0.

OFXR's static session/swapchain maps used to release remaining sessions on DLL
shutdown, invoking overlay cleanup and GPU-resource destructors when downstream
layers could already have destroyed their own registries. The layer registry
now has process lifetime. Explicit xrDestroySession and xrDestroyInstance still
perform normal cleanup; abandoned live sessions are left to OS process cleanup.
This deliberately retains abandoned objects rather than invoking runtime/GPU
APIs under loader lock. Applications must destroy their OpenXR objects before
unloading the layer during a continuing process, as before.

The regression test verifies explicit erasure destroys one session and late
CRT cleanup invokes zero downstream callbacks. Substituting an ordinary static
registry reproduces exit code 71. Existing layer lifecycle tests verify normal
explicit cleanup remains functional. Actual Silent Hill f retest is still needed;
the dump does not explain why process shutdown began at that point.

## The Outer Worlds 2 measurements

Source: 19:46:53 OFXR flight log, PID 19404, matched against Cheeky's timestamps.
Cheeky turns on near 19:48:10 and off at 19:48:40. Comparison windows are 47–72 s
(off) and 82–107 s (on) from the OFXR log filename time (subsecond origin is
approximate). Each window is 25 seconds and excludes the toggle.

| Metric | Off | On |
| --- | ---: | ---: |
| Application frame rate | 32.04/s | 33.04/s |
| Nonempty presenter submissions | 79.44/s | 79.52/s |
| New queued submissions | 64.12/s | 66.12/s |
| Repeated submissions | 15.32/s | 13.40/s |
| Mean application wait-frame | 17.61 ms | 15.68 ms |
| Mean synthesis-fence wait within wait-frame | 17.58 ms | 15.65 ms |
| Mean synthesis GPU interval | 2.53 ms | 2.48 ms |
| Mean application end-frame | 0.57 ms | 0.56 ms |
| Mean presenter software pacing wait | 10.23 ms | 10.25 ms |
| Presenter gaps >16.7 ms | 223/1985 | 204/1987 |

No negative OFXR results were recorded. No DLSS guide input records were
recorded. Cheeky's present source contains no OFXR guide-publication integration;
the standalone layer configuration cannot supply this by itself.

The fence wait includes time outside the measured synthesis GPU interval.
Likely contributors include earlier game work on the same queue, history copies,
queue dependencies, CPU scheduling and wakeup delay. The existing log cannot
separate them, so the new calibrated queue-delay counter measures the interval
before GPU synthesis starts. Do not sum overlapping CPU/GPU intervals or treat
a shorter GPU stage as an equal-sized FPS gain.

## Improvement priorities

1. Measure and reduce submission-to-start delay before changing optical-flow
   quality. Consider earlier synthesis submission or bounded CPU/GPU overlap,
   but preserve the exclusive history leases and destination lifetime fences.
   Removing the frame-start gate alone risks backlog, dropped synthesis and
   higher latency; it is not included in the crash fix.
2. Investigate presenter deadline skips. Upstream advances an absolute schedule
   and also skips deadlines less than half a period after handover. Runtime
   end-frame tails and timer overshoot can reduce throughput. Any adjustment
   needs both gap distributions and repeat counts; reaching 90 submissions/s
   by repeating more frames is not a game-FPS improvement.
3. A producer-side DLSS guide bridge could bypass OFA where inputs match and
   are valid, but requires identity, stereo and queue synchronization integration.
   It is not enabled merely by the existing menu setting.
4. Benchmark without image captures. Keep scene, resolution, headset rate and
   Cheeky state controlled. The observed +3.1% is one run, not a universal gain.

No speculative pacing or rendering-policy changes accompany this crash fix.
