# Silent Hill f queue submission review

Reviewed the existing 22:45:18 run (PID 29700), archived under
`build/run-analysis-20260919-2245`, and the current D3D12/layer implementation.
No additional game run was needed for this review. The implementation follow-up
below describes the subsequent runtime change.

## What the queue measurement means

The measured delay is from queue clock calibration immediately before NVIDIA
pack submission to the first timestamp in that command list. It includes work
already queued, including application rendering and OFXR history copies. It is
not a measurement of removable OFXR overhead. The existing log cannot partition
the 10.15 ms off / 10.27 ms on delay between those contributors.

History capture copies the application image on release, before passing release
downstream. Pair submission occurs at end-frame, after projection mappings and
private output ownership are known. Packing must read the completed history
image. A separate graphics queue would still need its producer fence; removing
that dependency or reading the released application image would be incorrect.
Submitting the entire pair at release would also lack the current end-frame
projection metadata and confirmed output ownership.

## Concrete optimization target: unused history captures

In the complete off window, 30–60 seconds:

| Application swapchain | Size | Successful releases | Synthesis inputs |
| --- | --- | ---: | ---: |
| 2337588198832 | 6800 × 3468 | 1140 | 1140 |
| 2337588199376 | 3200 × 1800 | 1140 | 0 |

Both have eligibility result 0 and three history slots. The release path attempts
history capture for both while armed, irrespective of which projection mapping
will be submitted. The smaller image uses format 91 (four bytes per pixel).
Each successful full-image copy therefore copies 23.04 MB of image payload;
at 38 releases/second that is 875.52 MB/s of payload, with both reads and writes.
These are size/rate estimates, not measured memory traffic: the flight log does
not record individual history-copy success or GPU duration.

Private output waits averaged only 37.6 microseconds across 2280 waits in this
window. Pair submission CPU scopes averaged 308.3 microseconds across 1140 pairs.
Neither supports claiming a multi-millisecond CPU submission fix.

Selective capture is the next concrete implementation candidate. It needs a
policy based on observed projection use, with explicit handling for a swapchain
that becomes a projection source after being skipped. That first changed frame
must use the existing safe bypass/re-prime behavior; it must never reuse a stale
capture. Startup, changing projection sources, multiple projection swapchains,
reused images, disarm/re-arm and pipelined applications require coverage before
enabling such a policy. A size-based exclusion alone is not valid.

## Decision

Keep the tested runtime installed. There is no demonstrated safe fence removal
or simple earlier-submission change that eliminates the measured queue delay.
Prioritize selective history capture over speculative queue changes. Any speedup
remains unmeasured; the existing successful run is sufficient to identify this
target, but cannot establish the performance of a future implementation.

## Implementation follow-up, September 20

After the first valid projection mapping, non-pipelined D3D12 sessions skip
history capture for swapchains never observed as projection sources. Startup
continues to capture all eligible images. Projection use belongs to the swapchain
object, not its numeric handle; a destroyed/recreated handle inherits no state.
Once a source has been used, capture remains enabled for its lifetime. This
conservative policy preserves returning projections and reused-image frames.
D3D11 and pipelined presentation retain capture of all eligible swapchains.
Session bookkeeping reset disables selection until mappings are learned again.

A skipped release still performs downstream release and normal ownership
bookkeeping, including clearing the previous capture and motion-vector ticket.
New projection sources are marked before layout quarantine or preparation can
bypass a frame. They therefore recover through a fresh capture and the existing
prime path, rather than synthesizing from a stale pre-selection image.

Flight operation `history_capture_skipped` records each skipped capture:
result=0, a=application swapchain handle, b=released image index, c=0. This reports
avoided capture attempts, not GPU time saved. Existing synthesis fences remain.

The `capture-selection` layer test uses two real D3D12 history resources under
the fake OpenXR runtime. It captures both at startup, repeatedly releases an
unused source, then switches projection to that source without releasing a new
image. It requires passthrough both on the switch and after quarantine, followed
by recovery to current/synthetic output once fresh images arrive. Existing
split-eye, pipelined, reused-image, disarm and lifetime tests remain applicable.

Release build and all 45 CTest cases passed, as did the NVIDIA hardware suite.
The new source-change regression fails against the preceding installed DLL.
After The Outer Worlds 2 closed, installation completed on September 20 at
09:41:20 into `RuntimeLayer/v116`. The previous DLL was backed up with suffix
`.before-selective-capture-20260920-094120.bak`. Installed and built SHA256 match:
`AF123FC14E55BFE5E466999ACE83AAFDE66B4A430571077F9D92B8AE5ECFF00F`.
No in-game performance gain is claimed.
