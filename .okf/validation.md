---
type: Reference
title: Performance evidence and validation
description: Verified tests, observed game results and limits on comparisons.
tags:
- ofxr
- performance
- fork
timestamp: '2026-09-20T00:00:00+09:00'
---

# Automated validation

50 CTest cases passed, plus the NVIDIA hardware suite. Coverage includes
resource lifetime, protected overlap boundaries, selective capture, recorder
rotation, scheduling overrides and output admission cancellation/retry/reuse.
Hardware cancellation coverage checks pending GPU work and untouched outputs.

# Game evidence, September 20, 2026

| Observation | Result | Limit |
|---|---|---|
| TOW2 original versus overlap, equal 23-second windows | 25.00 versus 38.83 app FPS; p95 46.423 versus 30.752 ms | Windows precede recorded CVar changes; scene workload is uncontrolled |
| SHf latest build with Cheeky, final analyzed 115 seconds | 44.66 app FPS; p95 22.996 ms; no >40 ms intervals | No matched old-build baseline |
| TOW2 latest build, Cheeky on/off settled sections | 39.96/39.14 app FPS; p95 33.667/40.117 ms | Different scenes and unequal windows; not causal proof |

In TOW2's settled sections, GPU stage intervals sum to about 2.38 ms in both
states. Queue delay averages 12.60 ms on / 14.48 ms off and includes preceding
work. Both states have large hitches: the worst 558 ms interval contains about
2.11 ms of recorded application OpenXR calls, locating most of that pause
between calls without identifying its source.

The latest SHf/TOW2 traces have no missing sequence IDs or negative OFXR results.
They lack explicit session/instance destruction records, so do not establish
orderly OpenXR teardown. Early flow executed in both games; its incremental
speedup is unproven. Adaptive is the supported default, not universally superior
 to fixed overlap. No visual-fidelity claim or universal comparison to djules
V185 is supported.

# Evidence locations

Local archives (ignored): `build/run-analysis-20260920-1817`,
`build/run-analysis-20260920-1914`, `build/run-analysis-20260920-1919`.
Portable detailed notes: repository `docs/SCHEDULING_COMPARISON.md`,
`docs/EARLY_OPTICAL_FLOW_20260920.md`, `docs/FORK_STATUS.md`.
See [operations](/operations.md) for log interpretation.
