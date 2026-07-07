# Project Status - ull-md-engine

## Project Goal

Build a staged performance-engineering platform that begins with a low-latency market-data pipeline and progressively develops capabilities in: 

- lock-free concurrency
- tail-latency engineering
- CPU cache and scheduler behavior
- Linux profiling
- hardware-counter analysis
- memory hierarchy
- NUMA
- SIMD and compiler optimization
- GPU acceleration
- CPU/GPU pipeline performance
- multi-GPU communication
- distributed AI-infrastructure performance analysis

The project uses a bottleneck-driven methodology:
```text 
question
  -> hypothesis
  -> controlled experiment
  -> measurement
  -> evidence
  -> optimization
  -> validation
```
The project is no longer primarily a queue implementation. It is a staged performance-engineering portfolio demonstrating concurrency correctness, lock-free design, cache behavior, scheduler effects, tail latency, overload semantics, profiling, and bottleneck-driven experimentation.

---

## Related Documents

- `docs/roadmap.md`
- `docs/design/design_requirements.md`
- `docs/design/performance_analysis_platform.md`
- `docs/design/benchmark_result_schema.md`
- `docs/design/v0_3_concurrency_architecture.md`
- `docs/design/disruptor_component_map.md`
- `docs/design/mpsc_memory_ordering.md`
- `docs/experiments/`

---

## Tagged Roadmap 

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

Detailed roadmap: `docs/roadmap.md`.

---

# v0.1 - Market Data Pipeline 

## Goal 

Runnable, measurable low-latency market-data engine 

## Completed

- UDP receiver over loopback
- fixed-size custom binary message format 
- SPSC lock-free ring buffer 
- monotonic tick abstraction 
- latency histogram with p50 / p99 / p999
- single consumer pipeline 
- end-to-end UDP benchmark 
- unit tests 
- release/dev scripts 
- README baseline 
- v0.1 tag 

## Main Deliverable 

- `SpscRing`
- `UdpReceiver`
- `LatencyHist`
- `simple_binary::Msg`
- `udp_pipeline_bench`
- benchmark output with p50 / p99 / p999 

Status: Complete.

---

# v0.2 — CPU Performance Experiments

## Goal

Isolate important sources of CPU latency and jitter.

## Completed

### 1. Cache and False Sharing

- added padded vs unpadded SPSC ring
- verified cache-line separation
- implemented false sharing microbenchmark
- observed ~4x improvement from padding on M2 Pro
- documented results

Docs:
- `docs/experiments/false_sharing.md`


---

### 2. Busy Polling vs Blocking

- implemented `BlockingQueue`
- compared:
  - busy polling SPSC ring
  - mutex + condition_variable blocking queue
- observed busy polling has lower p50 / p99 / p999
- documented scheduler wakeup overhead

Docs:
- `docs/experiments/busy_vs_blocking.md`
- `docs/experiments/busy_vs_blocking_profile.md`

---

### 3. Thread Affinity

- implemented macOS affinity hint support
- tested:
  - default
  - same affinity group
  - split affinity group
- observed macOS affinity is weak / non-deterministic
- same affinity generally worsens tail latency
- documented platform limitation

Docs:
- `docs/experiments/thread_affinity.md`

---

### 4. Profiling / Call Graph Analysis

- used gperftools / pprof
- generated call graph visualization
- identified hot paths:
  - `try_push`
  - `record_latency`
  - `LatencyHist::add`
  - atomic load/store
- classic Linux perf/flamegraph not fully implemented on macOS
- call graph analysis considered sufficient for v0.2

---

## v0.2 Status

Status: Completed.

Linux `perf` and a real Linux FlameGraph workflow are not part of v0.2.
They move to v0.4.

---

# v0.3 — Concurrent Pipeline Architecture 

## Current progress

Completed:
- initial bounded MPSC ring with per-slot sequence protocol 
- spinning `push()` baseline 
- non-blocking `try_push()` using CAS reservation 
- MPSC correctness tests 
- MPSC benchmark with `push` and `try_drop` modes 
- capacity sensitivity experiment 
- producer push latency instrumentation 
- sequence gap detection for drop-on-full mode 
- overload policy notes for market-data correctness 
- head/tail padding experiment for MPSC ring 

- added `SingleProducerEventPipeline` threaded correctness test and baseline benchmark.
- observed that larger pipeline capacity increases queue residency latency without clearly improving steady-state throughput.

Key findings:
- sustained overload turns queue capacity into backlog latency 
- smaller capacity lowers latency by bounding backlog 
- `try_drop` reduces producer waiting but creates sequence gaps 
- arbitrary drop-on-full is not sufficient for sequence-dependent market data without gap detection and resync 
- head/tail padding improves p99 latency moderately but does not remove producer contention on the shared `head_`

---

## Goal

Move from low-contention SPSC behavior to contention, overload, sequencing, and pipeline topology.

## Current Focus

v0.3 studies how concurrent pipelines behave under contention,
backpressure, overload, and fanout.

The current scope is intentionally limited:

- bounded MPSC behavior
- overload and sequence-gap semantics
- sequencer-controlled storage
- single-producer event pipeline
- shared-ring fanout exploration

The goal is not to build a full Disruptor clone.

---

## MPSC Ring

Completed:

- bounded MPSC ring
- per-slot sequence protocol
- spinning `push`
- bounded non-blocking `try_push`
- correctness tests
- producer-count benchmark
- configurable capacity
- configurable layout variants

Implemented files include:

- `include/ull/core/mpsc_ring.h`
- `include/ull/core/mpsc_ring_padded.h`
- `include/ull/core/mpsc_ring_cell_padded.h`
- `tests/test_mpsc_ring.cpp`
- `src/app/mpsc_bench_main.cpp`
- `scripts/run_mpsc_ring_rel.sh`

## Contention and Capacity

Completed:

- 1 / 2 / 4 / 8 producer experiments
- throughput reporting
- p50 / p99 / p999 latency reporting
- producer push-latency instrumentation
- capacity sensitivity experiments
- shared-head contention analysis
- head/tail padding experiments
- cell-level padding experiments

Findings:

- sustained overload turns queue capacity into backlog latency
- smaller capacity can lower latency by bounding backlog
- head/tail padding improves some latency metrics but does not remove
  contention on shared producer reservation state
- cell padding can reduce cache-line interaction among adjacent slots,
  but consumer count and shared reservation state remain important

Docs:

- `docs/experiments/mpsc_contention.md`
- `docs/experiments/mpsc_padding.md`

## Overload Semantics

Completed:

- `try_drop` mode
- published / dropped / consumed accounting
- sequence-gap detection
- market-data correctness notes

Findings:

- drop-on-full can reduce producer waiting and queueing latency
- arbitrary drops create sequence gaps
- sequence-dependent market-data streams require gap detection and
  recovery/resync before downstream state can be trusted

Docs:

- `docs/experiments/mpsc_overload_policy.md`

## Memory Ordering

Completed:

- acquire/release reasoning for MPSC publication
- claim vs publish explanation
- per-slot sequence protocol notes
- explanation of why consumers must not read partially written payloads

Docs:

- `docs/design/mpsc_memory_ordering.md`

## Sequencer Exploration

Started and partially completed:

- `Sequence`
- cache-line aligned sequence state
- `GatingSequences`
- `SingleProducerSequencer`
- claim / publish / consume lifecycle
- `SequencedRing`
- `SequenceBarrier`
- wait-strategy abstraction
- `SingleProducerEventPipeline`

Implemented files include:

- `include/ull/core/sequence.h`
- `include/ull/core/gating_sequences.h`
- `include/ull/core/single_producer_sequencer.h`
- `include/ull/core/sequenced_ring.h`
- `include/ull/core/sequence_barrier.h`
- `include/ull/core/wait_strategy.h`
- `include/ull/core/single_producer_event_pipeline.h`

## Single-Producer Event Pipeline

Completed:

- threaded correctness test
- baseline benchmark
- comparison against SPSC baseline benchmark
- queue-residency latency observation
- configurable wait-strategy support

Findings:

- the pipeline makes claim / write / publish / wait / read / consume
  explicit
- SPSC remains the simpler specialized point-to-point queue
- capacity/backlog dominates latency more than abstraction overhead in the
  tested workloads

Docs:

- `docs/experiments/sp_pipeline_baseline.md`

## Fanout Exploration

Completed:

- shared-ring fanout benchmark
- multiple consumers reading one published sequence stream
- producer backpressure through `GatingSequences`
- affinity-mode experiment for fanout benchmark

Findings:

- producer writes each event once
- all consumers observe the same ordered stream
- producer capacity is constrained by the slowest consumer
- throughput decreases as consumer count increases
- current gating scan is O(number of consumers)
- macOS affinity hints did not materially improve throughput or tail
  latency in the 4-consumer fanout runs

Docs:

- `docs/experiments/sp_pipeline_baseline.md`
- `docs/design/v0_3_concurrency_architecture.md`
- `docs/design/disruptor_component_map.md`

## Remaining v0.3 Scope

Remaining v0.3 work should stay intentionally limited.

Recommended remaining tasks:

1. Finish one bounded sequencer-controlled ring/fanout comparison.
2. Complete the remaining v0.3 architecture documentation.
3. Stop before building a complete Disruptor clone.

Status: In progress.

---

# v0.4 - Linux Performance Laboratory

## Goal

Build a reproducible Linux performance-analysis foundation.

Planned:

- strict CPU affinity
- system-topology capture
- `perf stat`
- `perf record`
- FlameGraph workflow
- canonical JSON benchmark results
- benchmark comparison tooling
- source/build metadata capture

Primary outcome:

```text
A reproducible Linux performance-analysis foundation.
```

Status: Planned.

---

# v0.5 - Memory + NUMA

## Goal

Analyze allocation, pages, locality, and remote-memory effects.

Planned:

- allocator jitter
- no-allocation hot path
- pre-touch
- page faults
- huge pages
- local vs remote NUMA memory
- topology-aware results
- burst traffic as a cross-cutting workload
- deterministic replay as a future workload source

Primary outcome:

```text
Evidence-based understanding of memory locality and tail jitter.
```

Status: Planned.

---

# v0.6 - SIMD / CPU Optimization

## Goal

Optimize real CPU hot paths using layout, vectorization, and compiler
techniques.

Planned:

- scalar baseline
- data-layout experiment
- SIMD path
- compiler optimization comparison
- cycles/item
- assembly inspection where useful
- branch and prefetch experiments where evidence justifies them

Primary outcome:

```text
One complete profile-to-optimization CPU case study.
```

Status: Planned.

---

# v0.7 - GPU Acceleration

## Goal

Add CUDA execution and study batching, launch, and transfer overhead.

Planned:

- CUDA workload
- CPU reference
- batch-size sweep
- transfer timing
- kernel timing
- CPU/GPU crossover analysis

Primary outcome:

```text
First heterogeneous performance experiments.
```

Status: Planned.

---

# v0.8 - Heterogeneous Pipeline Performance

## Goal

Optimize CPU/GPU overlap and end-to-end pipeline utilization.

Planned:

- pinned memory
- asynchronous transfer
- buffering
- CUDA streams
- Nsight Systems
- Nsight Compute

Primary outcome:

```text
End-to-end CPU/GPU pipeline optimization.
```

Status: Planned.

---

# v0.9 - Multi-GPU Communication

## Goal

Analyze collectives, topology, and communication/computation overlap.

Planned:

- NCCL collectives
- message-size sweeps
- GPU topology
- affinity experiments
- communication/computation overlap

Primary outcome:

```text
Communication and topology performance analysis.
```

Status: Planned.

---

# v1.0 - Distributed AI-Infrastructure Performance Platform

## Goal

Diagnose bottlenecks in distributed AI workloads using unified evidence.

Planned:

- distributed reference workload
- stage-level instrumentation
- bottleneck taxonomy
- bottleneck injection
- scaling analysis
- `ull-analyze` prototype

Primary outcome:

```text
Unified CPU, GPU, and communication bottleneck analysis.
```

Status: Planned.

---

## Current Immediate Next Steps

1. Finish the intentionally limited v0.3 sequencer/fanout comparison.
2. Complete the remaining v0.3 architecture documentation.
3. Prepare a Linux development and benchmark environment.
4. Begin v0.4 with:
   - strict CPU affinity
   - canonical benchmark JSON
   - system metadata capture
5. Add `perf stat` collection after the first JSON-emitting benchmark path
   is stable.
6. Build the first compare-results analysis script.

The immediate plan should not jump directly to CUDA. The project should
first establish:

```text
Linux
  -> reproducible measurements
  -> canonical results
  -> profiler evidence
```

before moving into later hardware layers.
