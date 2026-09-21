# Silent Hill f hitch review, September 20

Source: archived run `build/run-analysis-20260920-0948`. The 32 MiB recorder
rewind erased everything before elapsed 132.410 seconds. No complete Cheeky
off/on comparison or whole-run overlap-policy audit is possible.

For complete application end-frame intervals inside elapsed 140–180 seconds:

| Group | Intervals | Mean interval | Fence wait | Image release | End-frame |
| --- | ---: | ---: | ---: | ---: | ---: |
| Under 30 ms | 1540 | 23.049 ms | 0.300 ms | 0.406 ms | 4.722 ms |
| Over 40 ms | 65 | 46.131 ms | 8.218 ms | 8.878 ms | 18.804 ms |

For each interval, completed wait/release/end scopes were grouped by their end
timestamp. Fence waits are nested inside release/end scopes, so these columns
must not be added together. This is correlation, not proof that the fence itself
caused the hitch. The fence protects history/output lifetime while prior GPU work
finishes. Removing it would permit unsafe reuse.

Mean synthesis GPU time over the same window was 3.765 ms, queue delay 12.298 ms,
application throughput 41.20 FPS. The copy filter is active, but has not removed
the long-frame tail. The overlap controller uses a 24-frame mean; short clusters
of slow frames can remain below its 135%-of-target fallback threshold. Changing
that threshold without distinguishing GPU backlog from presenter pacing could
add switching stalls and does not establish a fix.

## Implemented diagnostic changes

- Recorder rollover now preserves every completed file and starts `.partN.log`
  segments. Timing and sequence IDs continue across files. The configured limit
  applies per file; total run storage grows. Creation failure retains the current
  handle and appends instead of truncating or repeatedly attempting creation.
- Added conditional begin/end timing for presenter capacity and request-completion
  waits. These expose the previously unpartitioned portion of `xrEndFrame` without
  changing presenter scheduling, queue limits or fence ownership.
- Added concurrent rollover coverage checking all 40,002 records survive exactly
  once, each normal segment obeys its size limit, and the startup record survives.
  A segment-name collision test checks existing data and continued recording survive.

This build fixes diagnostic data loss, not the demonstrated game hitching. The
new boundaries allow a subsequent ordinary run to distinguish presenter waiting
from the existing synthesis-fence measurements. The erased samples cannot be
reconstructed by changing the logger.

Release build and all 45 tests passed. Installed into `RuntimeLayer/v116` at
10:46:58 on September 20 after backing up the preceding DLL with suffix
`.before-retention-20260920-104658.bak`. Built and installed SHA256 matched:
`DF1355B69CA9FEDF93531762D80A9E26A0565B322ED00DC2E0DCBC51CB55BC7D`.
