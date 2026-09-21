---
type: Playbook
title: Build and diagnostic workflow
description: Build commands, deployment precautions and timing interpretation.
tags:
- ofxr
- performance
- fork
timestamp: '2026-09-20T00:00:00+09:00'
---

# Build and test

Use the repository README/build instructions to configure Windows VS2022 x64
and required SDK dependencies, then:

```powershell
cmake --build build --config Release --parallel 6
ctest --test-dir build -C Release --output-on-failure
$env:XRFG_TEST_NVIDIA = '1'
& .\build\Release\xrfg_d3d12_history_tests.exe
```

The hardware test requires a compatible NVIDIA GPU and driver. Restore/unset
the test environment variable afterward as appropriate. Local build logs are
`build/early-flow-tests.log` and `build/early-flow-nvidia-tests.log`.

# Deployment

Close games before replacing a runtime DLL. Back up the installed DLL, copy
`build/Release/ofxr/XR_APILAYER_XRFrameBridge_diagnostic.dll` into
`%LOCALAPPDATA%/OFXR Bridge/RuntimeLayer/v116/`, and verify SHA256. The tray's
`ofxr` DLL must match because arming can refresh the runtime copy.
Keep `OFXR_EXPERIMENTAL_PREPARE_AHEAD` unset. Select normal scheduling with
`scripts/Set-OFXRScheduling.ps1 -Mode Adaptive`. It applies to the next session;
Compare mode deliberately alternates schedules and should not remain selected.

# Diagnostics

Archive the base flight log and every `.partN.log`, plus Cheeky and UEVR logs,
before another run overwrites them. Cheeky transition timestamps identify state;
ignore stale GPU cost samples from inactive paths. Recorder rotation preserves
sequence IDs instead of discarding older portions.

`nvidia_gpu_total` spans pack through composition and can include output-admission
idle time. In stereo-array paths eye1 can include the gap too. Do not add nested
CPU waits or subtract CPU admission duration directly from GPU spans. Queue
delay includes earlier game work/history copies. Presenter submissions are not
proof of headset scanout, and repeats are not new generated frames.

Fidelity image capture is optional and dormant unless enabled; it is unnecessary
for performance analysis. [Validation](/validation.md) records current evidence.
