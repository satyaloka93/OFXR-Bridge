#!/usr/bin/env python3
"""Summarize labelled A/B phases from one OFXR flight log and its segments."""
import argparse
from pathlib import Path
import statistics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path, help='Original .log file, without .partN')
    args = parser.parse_args()
    base = args.log
    segments = sorted(base.parent.glob(base.stem + '.part*.log'),
                      key=lambda p: int(p.stem.rsplit('.part', 1)[1]))
    wanted = {'scheduling_experiment', 'app_end_frame', 'nvidia_gpu_total',
              'nvidia_queue_delay', 'presenter_capacity_wait', 'synthesis_frame_start_wait'}
    rows = []
    end = 0.0
    for path in [base, *segments]:
        with path.open() as source:
            for line in source:
                row = dict(field.split('=', 1) for field in line.split() if '=' in field)
                if 'ms' not in row:
                    continue
                row['time'] = float(row['ms']) / 1000
                end = max(end, row['time'])
                if row.get('op') in wanted and row.get('phase') != 'B':
                    rows.append(row)
    rows.sort(key=lambda row: row['time'])
    markers = [row for row in rows if row['op'] == 'scheduling_experiment']
    print('| Block | Mode | Usable seconds | App FPS | p95 ms | >40 ms frames | GPU ms | Queue ms | Presenter wait/frame ms |')
    print('|---|---|---:|---:|---:|---:|---:|---:|---:|')
    count = 0
    for index, marker in enumerate(markers):
        phase = int(marker['b'])
        if marker['a'] != '3' or marker['c'] != '1' or phase not in range(1, 5):
            continue
        stop = markers[index + 1]['time'] if index + 1 < len(markers) else end
        # Omit the first five seconds after switching gates, and incomplete blocks.
        start = marker['time'] + 5
        if stop - marker['time'] < 29:
            print(f'<!-- Block {phase} incomplete/interrupted; excluded. -->')
            continue
        sample = [row for row in rows if start <= row['time'] < stop]
        frames = [row['time'] for row in sample if row['op'] == 'app_end_frame' and row['phase'] == 'E']
        if len(frames) < 2:
            continue
        intervals = sorted((b - a) * 1000 for a, b in zip(frames, frames[1:]))
        def values(op, field):
            return [int(row[field]) / 1000 for row in sample if row['op'] == op]
        gpu = values('nvidia_gpu_total', 'b')
        queue = values('nvidia_queue_delay', 'a')
        wait = values('presenter_capacity_wait', 'dur_us')
        mean = lambda values: f'{statistics.mean(values):.2f}' if values else 'n/a'
        mode = {1: 'Original', 2: 'Overlap'}.get(int(marker['result']), 'Adaptive')
        duration = stop - start
        print(f'| {phase} | {mode} | {duration:.1f} | {len(frames)/duration:.2f} | '
              f'{intervals[int((len(intervals)-1)*.95)]:.2f} | '
              f'{sum(value > 40 for value in intervals)} | {mean(gpu)} | {mean(queue)} | '
              f'{sum(wait)/len(frames):.2f} |')
        count += 1
    if not count:
        print('No complete labelled comparison blocks found.')
    print('\nCompare both repetitions; scene, DLSS and Cheeky state must remain fixed. Waits overlap other timings.')


if __name__ == '__main__':
    main()
