# ULL Market Data Engine (ull-md-engine)

A staged ultra-low-latency market data pipeline focused on:

- lock-free data stuctures
- tail latency analysis (p50 / p99 / p999)
- CPU/cache-level performance behavior 
- scheduling and spin-wait strategies 

This project is designed to explore how low-level engineering decisions impact latency distribution in real-time systems.

---

## Key Features 

- SPSC lock-free ring buffer 
- synthetic market data feed (UDP loopback)
- latency histogram (p50 / p99 / p999)
- end-to-end pipeline benchmark 
- reproducible microbenchmarks 
---

## Engineering Experiments (v0.2)

### False Sharing 

Separating two hot atomic counters onto different cache lines reduced runtime by ~4x in a two-thread microbenchmark.
This demonstrates the severe performance penalty of cache-line contention.

---

### Busy vs Blocking 

- Busy polling achieves lower latency by staying in user space 
- Blocking introduces scheduler and wakeup overhead 

Flamegraph profiling confirms:
- busy -> CPU time spent in user-space polling 
- blocking - time spent in synchronization and scheduler paths 

---

### Thread Affinity 
On macOS (M2 Pro) 
- affinity hints are weak (not strict pinning)
- co-locating threads increases contention 
- default scheduling shows high variance 
---

### Spin Strategies 
Compared:
- pure spin 
- cpu_relax (pause/yield instruction) 
- thread_yield 
- exponential backoff 

Key findings:
- p50 latency is similar scross strategies 
- tail latency (p99/p999) differs significantly
- `thread_yield` reduces tail latency by ~30-40% on macOS 
- pure spin increase contention and tail jitters 

Insight:
> Tail latency is more sensitive to scheduling fairness than raw spin aggressiveness.

---


## Roadmap (Tagged Releases)

### v0.1 — MVP pipeline
- SPSC ring buffer
- synthetic market data feed
- latency measurement
- benchmark harness

### v0.2 — Engineering experiments
- false sharing 
- busy vs blocking 
- thread affinity 
- spin strategies 
- profiling / flamegraph 

### v0.3 — Advanced concurrency
- MPSC queue variants
- fanout topologies
- backpressure policies

### v0.4 — Memory optimization
- memory pool / slab allocator
- structure layout tuning
- branch / prefetch experiments

### v0.5 — NUMA & allocator control
- NUMA-aware pipeline
- allocator jitter control
- burst traffic modeling
---

## Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

## Development

### Unit tests (Debug + ASAN/UBSAN)
```bash
./scripts/test_dev.sh
```

---

## Experiments 

Detailed experiment reports:
- docs/experiments/false_sharing.md
- docs/experiments/busy_vs_blocking.md
- docs/experiments/thread_affinity.md
- docs/experiments/spin_strategies.md

---

## Key Takeaways 

- Tail latency (p99/p999) is more important than median latency 
- Cache-line layout can dominate performance (false sharing) 
- Scheduling behavior significantly impacts latency distribution 
- Spin strategy design is critical in low-latency systems 
- Optimal strategies are platform-dependent (macOS vs Linux)
---

## Status
- Phase 1(v0.1): Completed
- Phase 2(v0.2): In progress (experiments)
- Phase 3: Advanced concurrency and adaptive strategies 

