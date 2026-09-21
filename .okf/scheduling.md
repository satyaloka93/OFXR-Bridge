---
type: Reference
title: Scheduling and resource safety
description: Changes to CPU/GPU overlap, optical-flow admission and teardown.
tags:
- ofxr
- performance
- fork
timestamp: '2026-09-20T00:00:00+09:00'
---

# Implemented

- Process-lifetime registries avoid late CRT destruction calling downstream
  runtime/layer APIs after their teardown. Explicit OpenXR destruction retains
  cleanup. This fixes an identified shutdown hazard, not every possible crash.
- Bounded overlap moves the previous-synthesis gate from wait-frame to image
  release (or end-frame for reuse), allowing next-frame CPU preparation while
  protecting history and output resources. One synthesis transaction remains
  outstanding at most.
- Adaptive policy measures frame intervals, falls back to original admission,
  and retries overlap. Fixed overlap previously regressed SHf with Cheeky off.
- Selective history capture skips swapchains never used as projection sources
  once valid projection mapping is established; newly selected sources re-prime.
- Early NVIDIA preparation queues packing/flow before presenter capacity wait
  for eligible continuous, non-pipelined D3D12 frames with one projection mapping.
  Output writes still wait. Repeated/guide-assisted and other-backend paths retain
  earlier admission. Cancellation fences retain GPU resources until safe reuse.

# Rejected experiment

An extra synthetic swapchain per eligible source caused SHf and TOW2 VR injection
failure, likely an aggregate runtime allocation limit. It remains opt-in via
`OFXR_EXPERIMENTAL_PREPARE_AHEAD=1`, disabled by default. Current early flow needs
no additional runtime swapchains. Do not re-enable it for normal deployment.

# Limits

Overlap is not a separate asynchronous graphics queue. Waiting time can overlap
GPU execution and cannot be counted as entirely removable overhead. Cheeky does
not publish OFXR DLSS guides in the tested integration; a menu setting alone
cannot enable that integration. See [validation](/validation.md).
