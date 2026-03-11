# ULL Market Data Engine (ull-md-engine)

A staged ultra-low-latency market data ingestion / replay / benchmarking project.

## Roadmap (tagged releases)

### v0.1 — MVP pipeline
- SPSC ring buffer
- synthetic market data feed
- latency measurement
- benchmark harness

### v0.2 — Engineering experiments
- CPU affinity experiments
- false-sharing experiments
- perf / flamegraph profiling
- replay input mode
- CI benchmark artifacts

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
## Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

## Development

### Unit tests (Debug + ASAN/UBSAN)
```bash
./scripts/test_dev.sh
```

## Status

Phase 1 (v0.1) — Completed

Features:
- UDP loopback receiver
- fixed-size binary protocol
- SPSC lock-free queue
- latency histogram (p50/p99/p999)
- end-to-end pipeline benchmark

Next:
Phase 2 will focus on latency engineering experiments
(false sharing, affinity, spin strategies).
