# False Sharing Experiment

This document summarizes experiments exploring the impact of cache-line
false sharing on a simple SPSC queue implementation.

All experiments were performed on:

Machine:
- Apple MacBook Pro
- Apple M2 Pro
- macOS
- clang / C++20
- Release build

---

# Experiment A: Isolated False Sharing Microbenchmark

## Goal

Demonstrate the performance penalty of false sharing in a controlled
microbenchmark.

Two threads repeatedly increment separate atomic counters:

```C++
struct Unpadded {
    std::atomic<uint64_t> a;
    std::atomic<uint64_t> b;
};

struct Padded {
    alignas(64) std::atomic<uint64_t> a;
    alignas(64) std::atomic<uint64_t> b;
};
```

In the `Unpadded` case both counters reside in the same cache line.
In the `Padded` case they are separated to avoid cache-line contention.

Each thread performs:
- 200,000,000 fetch_add operations

## Results
mode=padded
time_ns=459213584

mode=unpadded
time_ns=1793238125

## Interpretation

The padded layout is approximately 3.9× faster.

This demonstrates a significant cache coherence penalty when two
threads repeatedly modify independent variables residing on the same
cache line.

Separating frequently modified atomics onto different cache lines
removes this contention.

# Experiment B: End-to-End SPSC Queue Latency Benchmark

Goal

Measure message latency in a minimal producer → queue → consumer
pipeline.

The benchmark measures:
- ts_send -> consumer processing latency


using a histogram with P50 / P99 / P999 statistics.

Two queue implementations were tested:
	•	SpscRing (padded head/tail)
	•	SpscRingUnpadded

Queue parameters:

```Shell
N = 2,000,000 messages
Warmup = 200,000
Histogram bucket = 50 ns
Max latency tracked = 20 ms
```

## Sample Runs

### Padded
p50_ns ≈ 2.46–2.51 ms
p99_ns ≈ 3.10–3.97 ms
p999_ns ≈ 3.16–4.11 ms

### Unpadded
p50_ns ≈ 1.4–3.3 ms
p99_ns ≈ 1.9–7.8 ms
p999_ns ≈ 2.0–7.8 ms

### Interpretation

Unlike the microbenchmark, the end-to-end queue benchmark does not
show a consistent improvement in median latency from padding.

However:
	•	The unpadded layout exhibits significantly larger variance
	•	Tail latency (P99 / P999) is often less stable

This suggests that in a realistic pipeline:
* queue dynamics
* OS scheduling
* producer/consumer overlap

can dominate the direct cost of cache-line contention.

Padding still improves structural isolation of hot fields but may not
translate directly into lower end-to-end latency in all workloads.

⸻

Takeaways
	1.	False sharing can impose large penalties in highly concurrent
write-heavy workloads.
	2.	Microbenchmarks are useful to isolate hardware effects.
	3.	End-to-end system benchmarks may behave differently due to:

	•	scheduling
	•	queue backlog
	•	pipeline dynamics

	4.	Careful benchmarking methodology is required when evaluating
low-latency systems.


