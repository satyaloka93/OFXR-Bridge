# Optional HUD panel pixel generation

Local patch on djules75 OFXR 0.2.14.2 (V557), commit
f64d4b0aa7e15747bc2060c8f962b830a53b02ab. No other upstream updates imported.

The tray has **HUD panel frame generation (restart game)**. Defaults on for
upstream-compatible behavior. Turning it off sets `panel_frame_generation=0`
in the tray settings and generated `[ofxr]` runtime INI. Re-arm preserves it.
The layer reads this once per session; restart the game after changing it.

Disabled: no optical-flow synth is armed/prepared for separate quad/cylinder
panels. Existing layer composition, image retention and pose interpolation stay
in place. Projection-eye frame generation remains enabled. Panel pixels update
at application cadence, which may make moving HUD content less smooth. This is
an optional performance/quality tradeoff, not a claim of equal output quality.
Do not replace this switch with `panel_corrections_off`: that gate also controls
REAL VR eye and pose behavior.

Motivation: Cyberpunk native SBS submits a separate 3144x3000 HUD swapchain.
V557 forces full-resolution FidelityFX flow for panels irrespective of the eyes'
Nvidia50% selection. A natural gameplay trace measured this panel's synthesis
GPU range at mean2.485ms. Removing it may create useful GPU headroom; actual
headset benefit and HUD quality remain unverified until a new game test.

Diagnostics: presenter_transition result950, a=0 disabled / a=1 enabled, once
at session creation. When disabled, no panel arm/configure/pair761/763/762 events
should occur; two projection synth chains should remain during normal gameplay.

Validation: MSVC Release layer/tray; seven selected CTests passed: default HUD
on, HUD off with shallow/deep buffering, split-eye, cropped split-eye, SteamVR
presenter transition, tray defaults and round-trip settings. HUD tests move the
quad and check midpoint pose .25 between .2/.3 while preserving its image source
and generated eye frame. Fake-runtime resources are destroyed on teardown.

Local deployment 2026-10-29 18:47 KST: patched existing Downloads/OFXR-Bridge-v0.2.14.2
tray and layer, plus RuntimeLayer/v557 DLL; saved old files and both INIs first.
No Cyberpunk files or pacing settings changed. Panel FG off, deep_pipeline0,
Nvidia medium50%, recorderon. New DLL SHA256
5dff9a41d4c3a8a636e7f6f2d64d050e33c527dd6c01af0f2508e202547afc83.
Backup receipt under cyberpunk-vr-port/build/diagnostics/sbs-gpu-20261029/
ofxr-backup-20261029-184740. Restore those files with game/tray stopped to revert.
This is a local unsigned diagnostic patch, not an official OFXR release.

## Presenter priority backport, 2026-10-29 18:59

Backported only the runtime change from djules75
fe79e2c94940, V570: set continuous presenter
thread to THREAD_PRIORITY_HIGHEST, not realtime. No queue-depth, readiness-hold,
GPU-fence or pipeline changes. Receipt773 logs requestedpriority2, appliedpriority,
and GetLastError on failure. Failed priority request does not stop framegen.

Motivation from post-panel-patch run60–160s: 8523 measured pace waits, median
~.352ms elapsed-minus-requested, p951.260ms,p992.071ms;665>1ms,100>2ms,max3.044ms.
This elapsed residual includes bookkeeping/timer/work as well as CPU scheduling;
not proof priority is sole cause or fix. It is an existing upstream improvement
with a narrow change and measurable follow-up.

Build and seven selectedtests passed again. SteamVRfake-runtime callback now
asserts off-application-thread presenter calls have THREAD_PRIORITY_HIGHEST.
Deployed only layerDLL to usual Downloads package and runtimecache, backed up
priorpanel-onlyDLL. Config hash unchanged. DLL SHA256
452321390e725e1e8a0a83ec528736a3fa18bc75f2462291e8c8fb1f3632d79a.
Backup priority-backup-20261029-185937 under cyberpunkdiagnostics.
Next run verify773a2b2c0; compare missed frames and pacing residual distribution.
KeepNvidiaMedium50%,Hybridoff,panelFGoff,deep_pipeline0 for this test.

Backend choice is not established optimal. Djules0.2.14.2 has no nativeDLSS-G
menu/path; that belongs to otherbranch/builtinport. Hybrid is a guide-assisted
quality/performance tradeoff. FidelityFX/flowresolution comparison is the next
settings candidate after priority evaluation; do not conflate it with this patch.

## Independent eye synthesis queues, 2026-10-29

User rejected lower-quality FidelityFX/flow-resolution alternatives. Preserve
NVIDIA Medium, 50%, unidirectional, Hybrid off, shallow buffering, panel FG off.

Baseline PID33356, seconds60–160: 4,357 matched serial pairs; GPU ranges for the
two eye swapchains never overlap. Combined first-begin to last-end mean12.100ms,
p9515.012ms; per-eye pack~.080ms, flow/wait5.535/5.983ms. The flow ranges include
scheduling and contention, not just optical-flow hardware execution. The shared
high-priority synthesis queue puts eye2 pack after eye1's flow wait/composition.

Opt-in `[ofxr] independent_synthesis_queues=1` gives each native D3D12 single-slice
swapchain its own DIRECT queue at the same HIGH priority. Defaultoff. Allocation
failure falls back to the existing shared queue. Arrays/D3D11/Vulkan retain the
existing path. No shader, quality preset, capture, buffering or fence policy
changed. Each synth retains its queue; existing teardown drains it. Producer
acquire joins, history completion, current-copy deferral and consumer hand-over
joins remain mandatory. The NVIDIA accelerator may still serialize its work;
this permits overlapping eligible stages, not simultaneous optical-flow claims.

Tray settings parse/serialize the option, so re-arm retains it. No extra menu
item yet. Runtime events951: a=application swapchain,b=queue identity,c=1 own /0
shared;952:a=swapchain,b=queue creationHRESULT. Existing calibrated GPUspan logs
measure whether overlap occurs. `tools/analyze_eye_queue_spans.py` reports stages,
matched serial spans and queue identity. It does not report displayedFPS.

Validation: nine selected CTests passed; separate-eye tests at shallow/deep
depth also assert exactly two distinct queue identities. NVIDIA hardware probe
`xrfg_d3d12_history_tests.exe independent-queues` runs100 stereo pairs per mode,
both pairs submitted before consumer joins, deferred copies checked bytewise.
Generated output matches baseline byte for byte. Walltimes176ms/shared and167ms/
independent are a tiny synthetic smoke test, not a gameplay performance claim.

Important inherited limitation: the first probe used COMMON-state destinations
and the debug layer rejected rendering into COMMON, even in the shared baseline.
Source already documents intentional undefined behavior at the comment preceding
`expected_guide_eyes` in d3d12_frame_synthesizer.cpp (V148 barrier regression).
The queue-isolation probe therefore uses layer-owned RENDER_TARGET destinations,
with no debug errors; it does NOT validate upstream's runtime-owned COMMON-state
workaround. The live layer's state policy is unchanged. Do not claim this patch
fixes that separate issue or that the full runtime passes D3D12 validation.

[NVIDIA fence contract](https://docs.nvidia.com/video-technologies/optical-flow-sdk/nvofa-programming-guide/index.html):
D3D12 input fences guard input readiness; the output fence signifies completed
flow processing. All retained. No API calls were parallelized on CPU threads.

Deployment completed19:24 with prior files saved in
`cyberpunk-vr-port/build/diagnostics/sbs-gpu-20261029/eye-queues-backup-20261029-192350`.
Layer SHA256 fb24317f16c1f56c7bf4227262fd1f1d6a122683664e978204bc163574332633.
Tray SHA256 746c2e3b3a558f85a19dcefd2eab51072203e584a8ca7030b2abc270ec975439.
Both package/runtime DLLs verified and tray restarted; NVIDIA quality retained.
First live PID30816 confirms distinctqueues. Window60–120s versusPID33356:
combinedspan12.187→7.187ms, meanoverlap0→5.107ms, heuristicFPS82.784→86.777,
zero-present records126/5234→9/5172. Scenes andCheekysettings notcontrolled;
GPUoverlap established, preciseFPSgain notestablished. See mainrepoOKFrecord.
