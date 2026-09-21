# Earlier NVIDIA optical-flow submission

The continuous, non-pipelined D3D12 NVIDIA path can now queue internal packing
and optical flow before waiting for the preceding presenter's output capacity.
This applies to a pairable frame with one projection resource mapping. The
output-writing synthesis/composition command list still waits for capacity.
Repeated captures, motion-vector-assisted submissions and other backends retain
admission before preparation. Handoff retains the previous queue-depth bound.

No runtime swapchains are added. The separate experimental extra-swapchain path
remains disabled by default following the SHf/TOW2 injection failures.
Adaptive scheduling remains selected.

If admission is cancelled after internal GPU work was queued, a completion fence
protects work slots and history leases before retry or teardown. Output command
lists are not submitted on cancellation. The callback is synchronous and must
not reenter the synthesizer.

Validation: all 50 CTest cases passed. NVIDIA hardware tests additionally passed
pending-work cancellation, untouched-output checks during admission, cancellation
and retry of the same capture, and repeated-image admission. These establish
correctness under the exercised conditions, not an in-game performance gain.

For the next game run, retain existing settings and compare application frame
intervals, frame-time tails, queue delay and presenter waits. No image capture
or repeat scheduling comparison is required.

`nvidia_output_admission` measures the CPU admission callback after internal
packing/flow submission and before output command submission. Its fields are
a=current capture serial, b=pack-ready fence value, c=flow-completion fence value.
These GPU fence values are identifiers, not durations. The interval can overlap
GPU execution and must not be subtracted directly from GPU timestamps.

`nvidia_gpu_total` spans packing through composition and can now include an idle
gap while output admission is pending. For single-slice inputs, eye0 and
composition timings retain their individual execution boundaries. For stereo
array inputs, the eye1 interval ending at composition start can also include the
admission gap. Increased total/eye1 time alone does not establish increased GPU
execution cost.
