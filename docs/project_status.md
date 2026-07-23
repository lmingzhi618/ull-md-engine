# Project Status - ull-md-engine

## Project Identity

`ull-md-engine` is a long-term low-latency systems learning project built
around a bounded market-data processing pipeline.

It is not primarily a production trading platform. It is a staged systems
laboratory and portfolio project for learning, implementing, measuring, and
explaining low-latency engineering concepts.

The project should be presented as:

```text
serious systems self-study
+ engineering experimentation
+ implementation evidence
+ benchmark-driven learning
+ bounded reference market-data pipeline
```

It should not be presented as production HFT experience.

## Methodology

The project follows this loop:

```text
question
-> hypothesis
-> implementation
-> correctness test
-> benchmark
-> observation
-> documentation
-> reflection
```

The learning and reasoning are first-class deliverables, not side effects.

## Related Documents

- `AGENTS.md`
- `docs/roadmap.md`
- `docs/design/design_requirements.md`
- `docs/design/benchmark_result_schema.md`
- `docs/design/canonical_benchmark_json_v1.md`
- `docs/design/v0_3_concurrency_architecture.md`
- `docs/design/disruptor_component_map.md`
- `docs/design/mpsc_memory_ordering.md`
- `docs/experiments/`

## Roadmap Snapshot

| Version | Theme | Status |
| --- | --- | --- |
| v0.1 | Market Data Pipeline | Complete |
| v0.2 | CPU Latency Experiments | Complete enough |
| v0.3 | Concurrent Pipeline Architecture | In progress |
| v0.4 | Linux Performance Laboratory | Planned / beginning through JSON work |
| v0.5 | Memory Determinism + NUMA | Planned |
| v0.6 | CPU Hot-Path Optimization | Planned |
| v0.7+ | Advanced I/O and heterogeneous performance | Planned long-term |

The exact current state must be checked from code, tests, benchmarks, and docs.
This file is a status index, not the sole source of truth.

---

# v0.1 - Market Data Pipeline

## Goal

Build a runnable and measurable low-latency market-data pipeline.

## Completed

- UDP receiver over loopback
- fixed-size custom binary message format
- SPSC lock-free ring buffer
- monotonic tick abstraction
- latency histogram with p50 / p99 / p999
- single-consumer pipeline
- end-to-end UDP benchmark
- unit tests
- release/dev scripts
- README baseline
- v0.1 tag

## Main Deliverables

- `SpscRing`
- `UdpReceiver`
- `LatencyHist`
- `simple_binary::Msg`
- `udp_pipeline_bench`
- benchmark output with p50 / p99 / p999

Status: Complete.

---

# v0.2 - CPU Latency Experiments

## Goal

Isolate important sources of CPU latency and jitter.

## Completed

### Cache and False Sharing

- padded vs unpadded SPSC ring
- false-sharing microbenchmark
- cache-line layout documentation
- observed significant improvement from padding on the test machine

Docs:

- `docs/experiments/false_sharing.md`

### Busy Polling vs Blocking

- `BlockingQueue`
- busy-polling SPSC vs mutex/condition-variable queue
- scheduler wakeup overhead discussion

Docs:

- `docs/experiments/busy_vs_blocking.md`
- `docs/experiments/busy_vs_blocking_profile.md`

### Thread Affinity

- macOS affinity hint support
- default / same / split affinity experiments
- documented macOS affinity limitations

Docs:

- `docs/experiments/thread_affinity.md`

### Spin Strategies

- pure spin
- CPU relax / pause style wait
- yield
- backoff / adaptive variants

Docs:

- `docs/experiments/spin_strategies.md`

### Profiling

- gperftools / pprof experiments
- call-graph visualization
- hot-path identification
- Linux `perf` deferred to v0.4

Status: Complete enough for tagged release.

---

# v0.3 - Concurrent Pipeline Architecture

## Goal

Study how concurrent pipelines behave under contention, backpressure, overload,
sequencing, and fanout.

The goal is to finish the bounded Disruptor-style work already started, without expanding into a full production Disruptor clone.

## Completed Or Implemented

### MPSC Ring

- bounded MPSC ring
- per-slot sequence protocol
- spinning `push()` baseline
- bounded non-blocking `try_push()`
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

### Contention And Capacity

Completed experiments:

- 1 / 2 / 4 / 8 producer runs
- throughput reporting
- p50 / p99 / p999 latency reporting
- producer push-latency instrumentation
- capacity sensitivity
- shared-head contention analysis
- head/tail padding
- cell-level padding

Findings:

- sustained overload turns queue capacity into backlog latency
- smaller capacity can lower latency by bounding backlog
- head/tail padding improves some latency metrics but does not remove shared
  producer reservation contention
- cell padding can reduce cache-line interaction among adjacent slots, but
  producer count and shared reservation state remain important

Docs:

- `docs/experiments/mpsc_contention.md`
- `docs/experiments/mpsc_padding.md`

### Overload Semantics

Completed:

- `try_drop` mode
- published / dropped / consumed accounting
- sequence-gap detection
- market-data correctness notes

Findings:

- drop-on-full can reduce producer waiting and queueing latency
- arbitrary drops create sequence gaps
- sequence-dependent market-data streams require gap detection and resync before
  downstream state can be trusted

Docs:

- `docs/experiments/mpsc_overload_policy.md`

### Memory Ordering

Completed:

- acquire/release reasoning for MPSC publication
- claim vs publish explanation
- per-slot sequence protocol notes
- explanation of why consumers must not read partially written payloads

Docs:

- `docs/design/mpsc_memory_ordering.md`

### Sequencer Exploration

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

### Single-Producer Event Pipeline

Completed:

- threaded correctness test
- baseline benchmark
- comparison against SPSC baseline benchmark
- queue-residency latency observation
- configurable wait-strategy support

Findings:

- the pipeline makes claim / write / publish / wait / read / consume explicit
- SPSC remains the simpler specialized point-to-point queue
- capacity/backlog dominates latency more than abstraction overhead in the
  tested workloads

Docs:

- `docs/experiments/sp_pipeline_baseline.md`

### Fanout Exploration

Completed or recently implemented:

- shared-ring fanout benchmark
- multiple consumers reading one published sequence stream
- producer backpressure through `GatingSequences`
- affinity-mode experiment for fanout benchmark
- canonical JSON output path for `sp_fanout_bench`

Findings:

- producer writes each event once
- all consumers observe the same ordered stream
- producer capacity is constrained by the slowest consumer
- throughput decreases as consumer count increases
- current gating scan is O(number of consumers)
- macOS affinity hints did not materially improve throughput or tail latency in
  the tested fanout runs

Docs:

- `docs/experiments/sp_pipeline_baseline.md`
- `docs/design/v0_3_concurrency_architecture.md`
- `docs/design/disruptor_component_map.md`
- `docs/design/canonical_benchmark_json_v1.md`

## Current v0.3 Gaps

- Some v0.3 architecture documentation may still need final consolidation.
- The current sequencer, barrier, wait-strategy, pipeline, and fanout work
  should be completed as a bounded Disruptor-style pipeline.
- The work should not expand into a full production Disruptor clone, generic
  concurrency framework, or complex dependency graph.
- Canonical JSON should be stabilized before broader benchmark automation.
- Project docs should remain synchronized with actual code and tests.

## Recommended Next Smallest Step

Inspect current code and docs, then choose one bounded step from:

1. Finish the bounded v0.3 Disruptor-style sequencer/fanout pipeline story.
2. Stabilize `sp_fanout_bench` JSON output and document the command.
3. Start a minimal v0.4 benchmark metadata experiment.

Definition of done for v0.3:

```text
SPSC baseline
-> MPSC contention and overload
-> sequence-based claim/publish/consume
-> single-producer pipeline
-> limited fanout and gating
-> documented trade-offs and stopping line
```

Status: In progress.

---

# v0.4 - Linux Performance Laboratory

## Goal

Build a reproducible Linux performance-analysis foundation.

Planned capabilities:

- strict CPU affinity
- system-topology capture
- canonical JSON benchmark results
- source/build metadata capture
- `perf stat`
- `perf record`
- FlameGraph workflow
- result comparison tooling

Primary outcome:

```text
A reproducible Linux performance-analysis foundation.
```

Status: Planned / beginning through canonical JSON work.

---

# v0.5 - Memory Determinism + NUMA

## Goal

Analyze allocation, pages, locality, and remote-memory effects.

Planned capabilities:

- no-allocation hot-path validation
- allocator jitter experiments
- pre-touch and page-fault experiments
- huge-page notes where appropriate
- local vs remote NUMA comparison on suitable hardware
- topology-aware result metadata

Primary outcome:

```text
Evidence-based understanding of memory locality and tail jitter.
```

Status: Planned.

---

# v0.6 - CPU Hot-Path Optimization

## Goal

Optimize real CPU hot paths using layout, vectorization, and compiler
techniques.

Planned capabilities:

- scalar baseline
- data-layout experiment
- SIMD path where hardware supports it
- compiler optimization comparison
- cycles/item analysis
- assembly inspection where useful
- branch and prefetch experiments when evidence justifies them

Primary outcome:

```text
One complete profile-to-optimization CPU case study.
```

Status: Planned.

---

# Planned Long-Term Research Tracks

The following tracks are part of the owner's long-term learning plan:

- DPDK and kernel-bypass networking
- RDMA and low-latency network transport
- GPU acceleration
- heterogeneous CPU/GPU pipeline performance
- distributed AI-infrastructure performance analysis

They are not ignored, but they are staged after the current concurrency and
Linux performance foundations. Each requires explicit prerequisites,
hardware/environment support, a design note, and a bounded experiment before
implementation.

---

## Immediate Work Protocol

Before recommending the next task, inspect:

- code
- tests
- benchmark executables
- docs
- recent commits
- current git status

Then report:

- implemented state
- partial state
- missing state
- recommended next smallest step
- definition of done
