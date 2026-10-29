# Cyberpunk SBS RC1 — OFXR v0.2.14.2 + local performance patches

This is an **unofficial, opt-in test release**, based on
[djules75/OFXR-Bridge 0.2.14.2 (V557)](https://github.com/djules75/OFXR-Bridge/tree/0.2.14.2),
commit `f64d4b0aa7e15747bc2060c8f962b830a53b02ab`.
It is published in satyaloka93's fork, whose original parent is tig3rmast3r;
**this candidate is built from djules75 V557, not tig3rmast3r V557/V681**.
It does not update or replace the Cyberpunk VR port, Cheeky, or headset drivers.

## Changes

- **Independent eye synthesis queues**, opt-in: separate native D3D12
  single-slice swapchains can prepare and compose without sitting behind the
  other eye's NVIDIA optical-flow wait on one shared queue. Allocation failure
  falls back to the shared queue. Existing resource fences and image retirement
  remain. NVIDIA preset, resolution and interpolation shaders are unchanged.
- **Optional HUD panel frame-generation bypass**: removes optical-flow work for
  separate HUD/menu panels while keeping their placement and pose interpolation.
  Panel pixels then update at game cadence, so animated HUD content may look less
  smooth. Projection-eye frame generation remains enabled.
- **Presenter thread priority**: backports the runtime change from djules75
  [fe79e2c94940](https://github.com/djules75/OFXR-Bridge/commit/fe79e2c94940)
  (V570), using Windows HIGHEST thread priority, not realtime.
- Settings persistence, queue-identification telemetry, regression tests and a
  GPU-span analysis tool accompany the changes.

## Install and test

1. Download and extract **OFXR-Bridge-Cyberpunk-SBS-RC1.zip** to a new permanent
   folder. Do not overwrite another OFXR installation.
2. Close your game and **Exit** any other OFXR tray. Keep only one OFXR version
   active. This still uses upstream's RuntimeLayer/v557 cache; exiting before
   switching versions is necessary.
3. In the Cyberpunk VR port's F10 menu, turn **built-in frame generation OFF**.
   Use the port's native SBS path. Leave your known-working game pacing settings
   unchanged; this package does not configure the port.
4. Run **Apply-Cyberpunk-Test-Profile.cmd** once. It backs up your existing OFXR
   tray settings, applies the candidate profile, then starts this package's tray.
   For later launches, run **OFXRBridgeTray.exe** directly.
5. Start SteamVR/OpenXR and launch Cyberpunk normally. Play naturally; no special
   stationary or bike-only test is required. Keep resolution, DLSS, ray tracing,
   Cheeky and headset refresh rate consistent when comparing builds.

The provided profile is intended for **NVIDIA optical-flow-capable hardware**:
NVIDIA Medium, 50% input scale, 2X, unidirectional, Hybrid off, deep pipeline off,
single-swapchain rings off, HUD panel FG off, independent synthesis queues on,
and diagnostics on. It preserves unrelated tray settings. You can adjust settings
in the tray afterward; don't rerun the profile helper unless you want these test
settings reapplied. No DLSS-G backend is added by this release.

The helper changes `%LOCALAPPDATA%\OFXR Bridge\tray.ini`. The normal OFXR tray
then manages its own OpenXR registration/runtime cache as upstream does. No admin
privileges should be needed. The package does not disable other vendors' layers.

## Roll back

Close the game and Exit this OFXR tray. Run **Restore-Previous-Profile.cmd** to
restore the settings saved before the first profile application (or remove the
created settings file if none existed). Then launch your previous OFXR tray from
its original folder; it restores that version's runtime files when it arms.
Do not delete the candidate folder while its tray is running.

## Evidence and limits

Tested locally with Cyberpunk VR port 0.1.9 native SBS, SteamVR/PSVR2, RTX4090,
NVIDIA Medium/50%. Other runtimes, headsets, GPUs and games are not validated.
Nine selected regression tests passed, including split eyes, HUD on/off and
buffering-depth cases. A hardware probe processed 100 stereo pairs per mode:
generated eye images matched the shared-queue baseline byte for byte, and
held-back real-frame copies were correct.

In one natural-play comparison (same 60–120-second log window), adding independent
queues to the panel/priority build changed combined eye-generation GPU time from
12.19 to 7.19 ms, zero-presentation compositor records from126/5234 to9/5172,
and OFXR's estimated delivery from82.8 to86.8 FPS. Scenes and Cheeky state were
not controlled. The trace confirms GPU overlap; it does **not** establish a
universal FPS improvement. OFXR's delivery metric is a heuristic, not an exact
count of uniquely displayed images.

**Known limitations:** this inherits upstream's documented COMMON-state rendering
workaround for runtime targets. The hardware queue-equivalence probe used valid
layer-owned render-target states and does not certify that inherited workaround.
No change to that policy is included. The HUD bypass is a smoothness tradeoff.
The independent-queue option is experimental and may increase contention on
other hardware. It currently has no tray menu item; the helper enables it.

**VRAM:** a tester observed19–21GB in fpsVR. The available short process-local
trace plateaued around14.8GiB; total GPU usage includes SteamVR and other apps.
There is no confirmed OFXR leak, but neither a long-session leak nor excessive
memory use has been ruled out. This release is not advertised as a VRAM fix.

## Report results

Please include headset/runtime, GPU/driver, refresh rate, per-eye resolution,
port version, DLSS/RT settings, HAGS setting, Cheeky on/off, visual artifacts,
perceived smoothness and whether VRAM levels off or keeps increasing.
Attach the matching timestamped `ofxr-bridge-flight-*.log` from
`%LOCALAPPDATA%\OFXR Bridge\RuntimeLayer\v557` and `ofxr_bridge.ini`.
Include fpsVR history if available. Logs may contain executable paths; review
before posting. Existing recorder limits still apply; copy the relevant log
before another long run. Do not mix logs from different OFXR branches.

Source, license notices and build instructions are supplied through the tagged
repository and packaged docs. `SHA256SUMS.txt` verifies the release files.
