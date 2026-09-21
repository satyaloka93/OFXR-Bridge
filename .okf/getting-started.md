---
type: Reference
title: OFXR performance fork
description: Upstream basis, scope and navigation for the tested performance fork.
tags:
- ofxr
- performance
- fork
timestamp: '2026-09-20T00:00:00+09:00'
---

# Scope

Based on upstream v0.2.1 / V116 at `dad56ac`. Internal runtime identity remains
V116; this fork is not an upstream release. Upstream already incorporates djules
presenter contributions; see repository `docs/PR3_INTEGRATION.md` for attribution.
No controlled comparison proves superiority over the separate djules V185 build.

The work targets SHf shutdown safety and SHf/TOW2 performance with Cheeky.
Visual fidelity has not been assessed. UEVR performance CVar edits are outside
this fork's scope.

- [Scheduling and lifetime guarantees](/scheduling.md)
- [Measured results and their limits](/validation.md)
- [Build, installation and diagnosis](/operations.md)

Detailed implementation reports live in repository `docs/`. Raw logs and local
builds remain in ignored `build/` directories and are not shipped in Git.
