# Flamegraph — Busy vs Blocking

## Environment
- MacBook Pro (M2 Pro)
- macOS
- Clang / C++20
- Release build
- gperftools CPU profiler

## Workload
- busy_vs_blocking_bench
- N = 500,000
- warmup = 50,000

## Busy Mode
Artifacts:
- busy.prof
- busy.svg / busy_flame.svg

Observations:
- hottest paths are concentrated in user-space polling and queue operations
- `try_push`, `try_pop`, timing, and histogram updates dominate CPU time

## Blocking Mode
Artifacts:
- blocking.prof
- blocking.svg / blocking_flame.svg

Observations:
- more time is spent in blocking queue synchronization paths
- condition variable / mutex related waiting and wakeup paths become more visible

## Interpretation
Busy polling keeps execution in user space and avoids scheduler-mediated wakeups.
Blocking mode introduces synchronization and wakeup overhead, which helps explain its worse median and tail latency in previous benchmarks.

## Takeaway
The flamegraph supports the benchmark results:
- busy mode spends CPU on active polling and queue processing
- blocking mode spends more time in synchronization and sleep/wakeup machinery
