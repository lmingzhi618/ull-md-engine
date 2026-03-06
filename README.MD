# ULL Market Data Engine (ull-md-engine)

A staged ultra-low-latency market data ingestion / replay / benchmarking project.

## Roadmap (tagged releases)
- v0.1 MVP: SPSC ring + synthetic feed + latency histogram
- v0.2 Engineering (B): affinity, false-sharing experiments, perf/flamegraph, replay input, CI artifacts
- v0.3 Advanced concurrency: MPSC/fanout topologies, backpressure policies
- v0.4 Memory optimization: pool/slab, layout tuning, branch/prefetch experiments
- v0.5 NUMA + allocator: NUMA-aware pipeline, allocator jitter control, burst modeling

## Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

## Development

### Unit tests (Debug + ASAN/UBSAN)
```bash
./scripts/test_dev.sh
```

## v0.1 Status

Implemented:
- monotonic tick abstraction with `ticks()` and `ticks_to_ns()`
- SPSC ring buffer with fixed power-of-two capacity
- latency histogram with percentile reporting in nanoseconds
- unit tests for ticks, ring buffer, and histogram
- simple benchmark harness (`ull_bench`)

Notes:
- v0.1 focuses on correctness, unit consistency, and testability.
- Performance optimization (false sharing, affinity, spin strategy) is planned for v0.2.
