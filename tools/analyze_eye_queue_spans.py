#!/usr/bin/env python3
"""Djules V557 recorder: per-chain stages and paired GPU intervals, not displayed FPS.

Use one session and a settled interval. Serial matching is valid only for the
application's two eye swapchains with matching capture serials. Queue timestamps
are calibrated onto QPC by OFXR. Wait portions include scheduling and contention,
not just optical-flow accelerator execution. A shorter union is not a latency
or displayed-FPS measurement.
"""
import argparse
import collections
import json
import re


def summarize(path, start=60, end=160):
    active = {}
    samples = collections.defaultdict(list)
    spans = collections.defaultdict(dict)
    queues = {}
    with open(path, errors='replace') as stream:
        for line in stream:
            f = dict(re.findall(r'(\w+)=([^\s]+)', line))
            if not {'ms', 'op', 'tid', 'a', 'b', 'c', 'result'} <= f.keys():
                continue
            if f['op'] == 'presenter_transition' and f['result'] == '951':
                queues[f['a']] = dict(queue=f['b'], independent=f['c'] == '1')
            if not start * 1000 <= float(f['ms']) <= end * 1000:
                continue
            tid, op = f['tid'], f['op']
            if op == 'synthesis_pair':
                if f.get('phase') == 'B':
                    active[tid] = f['a']
                elif f.get('phase') == 'E':
                    active.pop(tid, None)
            chain = active.get(tid)
            if chain is None:
                continue
            if op == 'nvidia_gpu_stages' and int(f['result']) > 0:
                for key, name in [('a', 'pack'), ('b', 'flow_wait'), ('c', 'second_slice_wait')]:
                    samples[chain + ':' + name].append(int(f[key]) / 1000)
            elif op == 'synthesis_gpu_span' and int(f['a']) > 0 and int(f['b']) >= int(f['a']):
                spans[int(f['c'])][chain] = (int(f['a']) / 1000, int(f['b']) / 1000)
    skipped = 0
    for pair in spans.values():
        if len(pair) != 2:
            skipped += 1
            continue
        x, y = sorted(pair.values())
        samples['two_eye_elapsed'].append(max(x[1], y[1]) - x[0])
        samples['eye_overlap'].append(max(0, min(x[1], y[1]) - y[0]))
        samples['signed_gap'].append(y[0] - x[1])
    result = {}
    for key, values in samples.items():
        values.sort()
        result[key] = dict(count=len(values), mean=sum(values)/len(values),
            p95=values[int(.95*(len(values)-1))], maximum=values[-1])
    return dict(window_seconds=[start, end], queues=queues, milliseconds=result,
                skipped_serials_not_exactly_two_chains=skipped)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('log')
    p.add_argument('--start', type=float, default=60)
    p.add_argument('--end', type=float, default=160)
    a = p.parse_args()
    print(json.dumps(summarize(a.log, a.start, a.end), indent=2))
