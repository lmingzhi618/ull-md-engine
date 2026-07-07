# ull-md-engine

`ull-md-engine` is a staged performance-engineering platform that begins with a low-latency market-data pipeline and progressively explores
CPU performance, concurrency, Linux profiling, memory hierarchy, NUMA, SIMD, GPU acceleration, heterogeneous pipelines, multi-GPU communication,
and distributed AI-infrastructure performance.

The project preserves its origin in low-latency market-data systems, but its long-term goal is broader:
build a portfolio of evidence-driven systems performance work.

The project focuses on bottleneck-driven performance analysis:

```text 
question 
  -> hypothesis 
  -> controlled experiment 
  -> measurement 
  -> bottleneck evidence 
  -> optimization 
  -> validation 
```

Future topics such as CUDA, NCCL, and distributed AI analysis are roadmap items. They are not implemented yet.

## Current Capabilities 

Implemented today:
- UDP loopback market-data pipeline 
- fixed-size binary message format 
- SPSC lock-free ring buffer 
- bounded MPSC ring buffer 
- per-slot sequence protocol 
- latency histogram with p50 / p99 / p999 reporting 
- false-sharing experiments 
- busy-polling vs blocking experiments 
- thread-affinity experiments on macOS
- spin-strategy experiments 
- hotspot and call-graph profiling with gperftools/pprof 
- MPSC producer-contention benchmarks
- queue-capacity sensitivity experiments 
- overload and drop-on-full experiments 
- sequence-gap detection for dropped market-data messages 
- padded and cell-padded ring layout experiments 
- early sequencer architecture exploration 
- single-producer event pipeline experiments 
- shared-ring fanout benchmark experiments 

Not implemented yet:
- full Linux `perf` workflow 
- real Linux FlameGraph workflow 
- NUMA optimization
- SIMD optimization 
- CUDA 
- Nsight profiling 
- NCCL 
- distributed AI workload analysis 

## Engineering Insights 

### False Sharing 

Separating hot atomic state onto different cache lines produced a substantial improvement in controlled two-thread experiments. The result shows how cache-line contention can dominate simple concurrent workloads. 

### Scheduler Interaction 

Busy waiting avoids blocking wakeup costs, but aggressive spinning can increase contention and tail jitter. On macOS, scheduler behavior and weak affinity hints make tail-latency results noisy.

### Queue Capacity 

Under sustained overload, larger queues can convert excess capacity into additional backlog latency without clearly improving steady-state throughput.

### Drop Policy 

Drop-on-full behavior can reduce producer waiting, but arbitrary drops can break sequence-dependent market-data correctness unless gap detection and recovery exist.

### MPSC Contention 

Padding shared metadata can reduce false sharing, but it does not remove fundamental contention on shared producer reservation state. 

### Fanout 

Shared-ring fanout makes ordering and backpressure explicit, but producer progress is constrained by the slowest consumer. More consumers also increase gating overhead and scheduler pressure.

## Performance Methodology 

Major experiments follow:
```text 
question 
  -> hypothesis 
  -> controlled variants 
  -> reproducible measurement 
  -> bottleneck evidence 
  -> optimization 
  -> before/after validation 
  -> documented limitations 
``` 
From v0.4 onward, new benchmarks are expected to support canonical JSON results for machine comparison and analysis.

## Roadmap 

| Version | Theme                                              | Status                             |
| ------- | -------------------------------------------------- | ---------------------------------- |
| v0.1    | Market Data Pipeline                               | Complete                           |
| v0.2    | CPU Performance Experiments                        | Complete enough for tagged release |
| v0.3    | Concurrent Pipeline Architecture                   | In progress                        |
| v0.4    | Linux Performance Laboratory                       | Planned                            |
| v0.5    | Memory + NUMA                                      | Planned                            |
| v0.6    | SIMD / CPU Optimization                            | Planned                            |
| v0.7    | GPU Acceleration                                   | Planned                            |
| v0.8    | Heterogeneous Pipeline Performance                 | Planned                            |
| v0.9    | Multi-GPU Communication                            | Planned                            |
| v1.0    | Distributed AI-Infrastructure Performance Platform | Planned                            |

See `docs/roadmap.md` for detailed goals, experiments, and exit criteria.

## Build 

```bash
cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build/rel
```

Debug build:
```bash 
cmake -S . -B build/dev -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev
```

## Test 

```bash 
./scripts/test_debug.sh
```
ASAN/UBSAN is optional and can be enabled separately when the local
toolchain supports it reliably.

## Example Benchmarks 

MPSC contention benchmark:
```bash 
./scripts/run_mpsc_ring_rel.sh push 4 500000 50000 1024 padded
```

Single-producer event pipeline benchmark:
```bash 
./build/rel/sp_pipeline_bench 1000000 1024
```

Shared-ring fanout benchmark:
```bash 
./build/rel/sp_fanout_bench 4 1000000 1024 default 50000
```

## Documentation 

- `docs/roadmap.md`
  Long-term project roadmap.
- `docs/design/design_requirements.md`
  Global requirements for implementation and experiments. 
- `docs/design/performance_analysis_platform.md`
  Performance-analysis methodology and platform design.
- `docs/design/benchmark_result_schema.md`
  Canonical machine-readable benchmark result model.
- `docs/project_status.md`
  Current implementation progress. 
- `docs/experiments/`
  Completed experiment reports. 
- `docs/design/`
  Architecture and correctness reasoning.


