# ull-md-engine Roadmap

## 1. Project Direction

`ull-md-engine` evolves from a low-latency market-data engine into a staged performance-engineering platform spanning:

* low-latency data movement
* lock-free concurrency
* CPU microarchitecture
* Linux performance analysis
* memory hierarchy and NUMA
* SIMD and compiler optimization
* GPU acceleration
* heterogeneous CPU/GPU pipelines
* multi-GPU communication
* distributed AI-infrastructure performance analysis

The project should preserve one continuous engineering story:

```text
market-data pipeline
    -> low-latency systems laboratory
    -> CPU performance engineering platform
    -> heterogeneous CPU/GPU performance platform
    -> distributed AI-infrastructure performance platform
```

The goal is not to collect technologies.

The goal is to apply one repeatable performance-engineering methodology across increasingly complex systems:

```text
question
  -> hypothesis
  -> controlled experiment
  -> measurement
  -> bottleneck evidence
  -> optimization
  -> before/after validation
  -> documented trade-off
```

---

## 2. Release Summary

| Version | Theme                                              | Primary Capability                                                           |
| ------- | -------------------------------------------------- | ---------------------------------------------------------------------------- |
| v0.1    | Market Data Pipeline                               | Build a measurable end-to-end low-latency pipeline                           |
| v0.2    | CPU Performance Experiments                        | Demonstrate cache, scheduling, spin, and profiling effects                   |
| v0.3    | Concurrent Pipeline Architecture                   | Study contention, backpressure, sequencing, and fanout                       |
| v0.4    | Linux Performance Laboratory                       | Build a reproducible Linux profiling and benchmark foundation                |
| v0.5    | Memory + NUMA                                      | Analyze allocation, pages, locality, and remote-memory effects               |
| v0.6    | SIMD / CPU Optimization                            | Optimize real hot paths using layout, vectorization, and compiler techniques |
| v0.7    | GPU Acceleration                                   | Add CUDA execution and study batching, launch, and transfer overhead         |
| v0.8    | Heterogeneous Pipeline Performance                 | Optimize CPU/GPU overlap and end-to-end pipeline utilization                 |
| v0.9    | Multi-GPU Communication                            | Analyze collectives, topology, and communication/computation overlap         |
| v1.0    | Distributed AI-Infrastructure Performance Platform | Diagnose bottlenecks in distributed AI workloads using unified evidence      |

---

# v0.1 — Market Data Pipeline

## Objective

Build a runnable and measurable data pipeline with stable latency reporting.

## Core Features

* UDP loopback receiver
* fixed-size binary message format
* SPSC lock-free ring buffer
* monotonic timestamp abstraction
* latency histogram
* p50 / p99 / p999 reporting
* single-consumer event path
* release benchmark scripts
* unit tests

## Performance Questions

* What is the baseline end-to-end latency distribution?
* Where is time spent across receive, enqueue, dequeue, and measurement?
* How stable is the benchmark across repeated runs?

## Exit Criteria

* benchmark executes reliably
* core modules are tested
* stable result format exists
* baseline performance is documented

Status: Complete.

---

# v0.2 — CPU Performance Experiments

## Objective

Isolate important sources of CPU latency and jitter.

## Core Experiments

### Cache-Line Contention

* padded vs unpadded hot counters
* false-sharing microbenchmark
* cache-line layout documentation

### Busy Polling vs Blocking

* lock-free busy polling
* mutex + condition-variable baseline
* scheduler wakeup analysis

### Thread Affinity

* platform-specific affinity experiments
* same-group vs split-group placement
* explicit documentation of macOS limitations

### Spin Strategies

* pure spin
* CPU relax / pause
* thread yield
* exponential backoff
* adaptive strategy

### Profiling

* call-graph analysis
* hotspot identification
* comparison of profiling limitations by platform

## Performance Questions

* When does cache-line contention dominate execution time?
* How much tail latency comes from scheduler interaction?
* Why can a less aggressive spin policy improve p99/p999?
* Which functions dominate CPU time on the hot path?

## Exit Criteria

Every experiment must contain:

1. question
2. hypothesis
3. controlled variants
4. reproducible command
5. measurements
6. interpretation
7. limitations

Status: Complete enough for tagged release.

---

# v0.3 — Concurrent Pipeline Architecture

## Objective

Move from low-contention SPSC behavior to contention, overload, sequencing, and pipeline topology.

## Core Features

### Bounded MPSC Ring

* per-slot sequence protocol
* producer claim vs publish separation
* blocking/spinning `push`
* non-blocking `try_push`
* producer-count scaling
* capacity sensitivity
* sequence-gap detection

### Backpressure and Overload

* bounded backlog
* drop-on-full behavior
* explicit correctness consequences
* sequence-dependent recovery requirements

### Contention Experiments

* 1 / 2 / 4 / 8 producers
* throughput and tail latency
* shared-head contention
* padded vs unpadded metadata

### Sequencer Exploration

* `Sequence`
* single-producer sequencer
* claim / publish / consume lifecycle
* gating sequence
* sequenced ring storage
* limited fanout exploration

## Performance Questions

* How does producer contention amplify tail latency?
* Why can larger queues increase latency without improving throughput?
* When does dropping reduce queueing latency but violate data semantics?
* What contention remains after removing false sharing?
* How should ordering be separated from storage?

## Exit Criteria

* MPSC correctness and overload behavior documented
* memory-ordering reasoning documented
* contention benchmark completed
* one bounded sequencer or fanout experiment completed
* no attempt to clone a full Disruptor unless a specific performance question requires it

Status: In progress.

---

# v0.4 — Linux Performance Laboratory

## Objective

Build the instrumentation and reproducibility foundation required for serious CPU, GPU, and AI-infrastructure performance work.

This phase is a prerequisite for later optimization work.

## Core Features

### 1. Strict CPU Affinity

Provide Linux support for:

* `sched_setaffinity`
* `pthread_setaffinity_np`
* CPU-set configuration from benchmark CLI
* system-topology capture

Required experiments:

* unpinned baseline
* producer and consumer on different physical cores
* producer and consumer on SMT siblings
* different NUMA nodes when hardware permits

Metrics:

* throughput
* p50
* p99
* p999
* maximum latency
* context switches
* CPU migrations

### 2. `perf stat` Integration

Collect at least:

* cycles
* instructions
* branches
* branch misses
* cache references
* cache misses
* context switches
* CPU migrations
* page faults

Derive where meaningful:

* IPC
* branch-miss rate
* cache-miss rate

Later experiments may add:

* LLC load misses
* dTLB load misses
* frontend stalls
* backend stalls
* architecture-specific events

### 3. `perf record` and FlameGraph Workflow

Add scripts for:

```text
perf record
  -> perf script
  -> stack collapse
  -> FlameGraph
```

Generated artifacts should identify:

* benchmark configuration
* binary
* git commit
* platform
* profiling command
* generated SVG

### 4. Unified Benchmark JSON

All benchmarks should support machine-readable JSON conforming to:

```text
docs/design/benchmark_result_schema.md
```

Human-readable console output may remain.

JSON becomes the canonical artifact for:

* comparison
* regression tracking
* chart generation
* future automated analysis

### 5. Benchmark Run Metadata

Capture:

* git commit
* dirty-tree status
* build type
* compiler
* compiler version
* compiler flags
* OS
* kernel
* CPU model
* core topology
* NUMA topology
* benchmark arguments
* warmup policy
* sample count
* timestamp

### 6. Analysis Scripts

Provide Python tooling to:

* load result JSON
* compare variants
* calculate deltas
* produce latency charts
* produce throughput charts
* correlate latency changes with hardware counters

## Performance Analysis Project A — Affinity and Scheduler Interference

### Question

How do CPU placement and scheduler movement affect tail latency?

### Variants

* unpinned
* pinned to different physical cores
* pinned to SMT siblings
* cross-NUMA where available

### Evidence

* latency distribution
* context switches
* CPU migrations
* cycles
* instructions
* IPC

## Performance Analysis Project B — Hot-Path Attribution

### Question

Which functions and code paths explain the observed latency distribution?

### Workflow

```text
baseline
  -> perf stat
  -> perf record
  -> FlameGraph
  -> identify dominant path
  -> targeted optimization
  -> before/after comparison
```

## Exit Criteria

* Linux benchmark environment documented
* strict affinity works
* at least one benchmark emits canonical JSON
* `perf stat` metadata is captured
* one FlameGraph-backed optimization study is documented
* analysis scripts compare at least two result files

---

# v0.5 — Memory + NUMA

## Objective

Study how allocation, page behavior, cache/TLB behavior, and memory locality affect throughput and tail latency.

## Core Features

### 1. Allocation Jitter

Compare:

* `new/delete`
* `malloc/free`
* preallocated fixed-size pool
* slab or freelist allocator
* optional `std::pmr`

Metrics:

* allocation latency
* p50
* p99
* p999
* max
* throughput
* page faults
* CPU time

Primary question:

> Why can dynamic allocation have modest average cost but severe tail-latency impact?

### 2. Hot-Path No-Allocation Mode

Provide explicit evidence that selected pipeline modes perform no dynamic allocation after initialization.

Possible validation mechanisms:

* allocator instrumentation
* test hooks
* profiler evidence
* explicit implementation invariant with tests

### 3. Page-Fault Experiment

Compare:

* lazy allocation
* pre-touch
* optional `mlock`

Measure:

* minor page faults
* major page faults
* startup latency
* steady-state tail latency

### 4. Huge-Page Experiment

Compare where supported:

* regular pages
* transparent huge pages
* explicit huge pages

Measure:

* dTLB-related counters
* throughput
* tail latency

### 5. NUMA-Aware Allocation

Support:

* CPU pinning by NUMA node
* memory allocation by NUMA node
* local CPU / local memory
* local CPU / remote memory
* producer and consumer placement across nodes

Tools may include:

* `numactl`
* `libnuma`

### 6. Topology-Aware Result Metadata

JSON results should include:

* NUMA node placement
* CPU set
* memory policy
* page mode

## Performance Analysis Projects

### Project A — Local vs Remote Memory

How much latency and bandwidth are lost when compute and memory placement diverge?

### Project B — Allocation-Induced Tail Jitter

Can preallocation reduce p999 and max latency even when throughput changes little?

### Project C — Queue Residency and Memory Pressure

How do queue capacity and working-set size interact with cache and TLB behavior?

## Exit Criteria

* at least one custom or preallocated allocator path
* page-fault experiment
* local-vs-remote NUMA experiment on suitable hardware
* JSON captures placement and memory policy
* one bottleneck explanation combines latency and hardware-counter evidence

---

# v0.6 — SIMD / CPU Optimization

## Objective

Optimize a real pipeline hot path and explain the improvement through microarchitectural evidence.

## Core Features

### 1. Scalar Baseline

Choose a concrete workload:

* binary message parsing
* field normalization
* filtering
* aggregation
* checksum or validation

The baseline must be simple, correct, and benchmarked.

### 2. Data-Layout Experiment

Compare:

* AoS
* SoA
* optional AoSoA

Measure:

* cycles/item
* instructions/item
* IPC
* cache misses
* branch misses
* throughput
* latency distribution

### 3. SIMD Implementation

Implement at least one architecture-specific optimized path:

* AVX2 preferred for x86
* AVX-512 when supported
* NEON for ARM comparison where useful

Retain a scalar reference implementation for correctness.

### 4. Compiler Optimization Study

Compare:

* `-O2`
* `-O3`
* `-march=native`
* LTO
* optional PGO

Record:

* build configuration
* performance change
* relevant generated assembly differences

### 5. Branch and Prefetch Experiments

Only add when driven by a concrete hypothesis.

Possible variants:

* branchy vs branchless
* likely/unlikely annotations
* manual prefetch

Do not retain optimizations without measurable benefit.

## Performance Analysis Project A — Scalar to SIMD

```text
baseline
  -> profile
  -> identify bottleneck
  -> change data layout or algorithm
  -> vectorize
  -> compare counters
  -> validate correctness
  -> validate performance
```

## Performance Analysis Project B — Throughput vs Tail Latency

Does a throughput optimization also improve tail latency, or does it create new burst and jitter behavior?

## Exit Criteria

* one real hot path optimized
* scalar and SIMD paths tested for correctness
* cycles/item and throughput reported
* hardware-counter explanation documented
* compiler configuration included in JSON

---

# v0.7 — GPU Acceleration

## Objective

Introduce CUDA and move from CPU-only analysis to heterogeneous execution.

The objective is not sophisticated AI modeling.

The objective is to study:

* GPU execution
* batching
* launch overhead
* transfer overhead
* CPU/GPU crossover
* GPU utilization

## Core Features

### 1. GPU-Capable Workload

Add a simple parallel stage:

* filtering
* normalization
* aggregation
* rolling or statistical transform

Pipeline:

```text
UDP or replay
  -> CPU preprocessing
  -> batch formation
  -> H2D transfer
  -> CUDA kernel
  -> result collection
```

### 2. Batch-Size Sweep

Test a broad range:

```text
1
8
32
128
1024
8192
```

Exact values may change by workload.

Measure:

* end-to-end latency
* queueing latency
* H2D latency
* kernel latency
* throughput
* GPU utilization

Primary question:

> Where is the latency/throughput knee between underutilized small batches and queue-heavy large batches?

### 3. CPU vs GPU Crossover

Run the same operation on:

* CPU
* GPU

Primary question:

> At what workload size does GPU acceleration overcome transfer and launch overhead?

### 4. CUDA Event Timing

Use GPU-native events for:

* H2D timing
* kernel timing
* D2H timing

Preserve host-side end-to-end timing.

### 5. GPU Metadata

Result JSON should capture:

* GPU model
* driver version
* CUDA version
* kernel configuration
* batch size
* transfer mode

## Performance Analysis Project A — Batch-Size Phase Diagram

Identify:

* CPU-bound region
* transfer-bound region
* launch-overhead region
* GPU-efficient region
* queueing-dominated region

## Performance Analysis Project B — CPU/GPU Crossover

Produce a performance curve showing where GPU execution becomes beneficial.

## Exit Criteria

* one CUDA kernel integrated
* CPU reference path retained
* batch sweep documented
* transfer and kernel timing separated
* result JSON includes GPU metadata

---

# v0.8 — Heterogeneous Pipeline Performance

## Objective

Optimize the complete CPU/GPU pipeline rather than isolated kernels.

## Core Features

### 1. Pageable vs Pinned Host Memory

Compare:

* regular host allocation
* pinned host memory

Measure:

* H2D latency
* D2H latency
* bandwidth
* CPU overhead
* end-to-end effect

### 2. Synchronous vs Asynchronous Transfer

Compare:

* blocking copies
* `cudaMemcpyAsync`

### 3. Multi-Buffer Pipeline

Implement double buffering or multi-buffering.

Example:

```text
batch N:      H2D -> compute -> D2H
batch N + 1:        H2D -> compute -> D2H
```

### 4. CUDA Streams

Compare:

* one stream
* two streams
* multiple streams where justified

Measure:

* overlap ratio
* exposed copy time
* exposed compute time
* GPU idle gaps
* CPU idle gaps

### 5. Nsight Systems Analysis

At least one project must diagnose:

* serialization
* launch gaps
* CPU stalls
* GPU idle time
* copy/compute overlap

### 6. Nsight Compute Analysis

At least one kernel should be analyzed for:

* occupancy
* memory throughput
* major warp-stall reasons
* SM efficiency
* arithmetic or memory intensity where useful

## Performance Analysis Project A — End-to-End Overlap Optimization

```text
serialized baseline
  -> timeline evidence
  -> pinned memory
  -> asynchronous copies
  -> buffering and streams
  -> improved overlap
  -> identify new bottleneck
```

## Performance Analysis Project B — GPU Starvation

Determine whether low GPU utilization is caused by:

* slow CPU preprocessing
* transfer serialization
* small batches
* launch gaps
* kernel inefficiency

## Exit Criteria

* pinned-memory comparison
* asynchronous transfer path
* at least one overlap design
* Nsight Systems report
* one Nsight Compute-backed kernel analysis
* JSON includes stage-level timing

---

# v0.9 — Multi-GPU Communication

## Objective

Study the communication layer that dominates large-scale AI workloads.

## Core Features

### 1. NCCL Benchmark Integration

Support:

* AllReduce
* AllGather
* ReduceScatter
* Broadcast

Run a message-size sweep.

Metrics:

* latency
* algorithm bandwidth
* bus bandwidth
* scaling efficiency

### 2. Topology Capture

Record:

* GPU placement
* PCIe hierarchy
* NVLink connectivity
* NUMA relationship
* topology artifacts such as `nvidia-smi topo -m`

### 3. Topology-Aware Placement

Compare favorable and unfavorable:

* CPU placement
* GPU selection
* NUMA placement

### 4. Collective Algorithm Analysis

Where observable or controllable, study:

* ring-like behavior
* tree-like behavior
* small-message latency
* large-message bandwidth

### 5. Communication/Computation Overlap

Implement useful compute overlapping with collective communication.

Measure:

* total communication time
* exposed communication time
* hidden communication time
* overlap percentage

## Performance Analysis Projects

### Project A — Collective Performance Curve

How does the bottleneck change from tiny messages to bandwidth-scale messages?

### Project B — Topology Penalty

How much performance is lost from poor CPU/GPU/NUMA placement?

### Project C — Exposed vs Hidden Communication

How much communication can be hidden behind compute, and what limits further overlap?

## Exit Criteria

* multi-GPU benchmark executed
* topology artifact captured
* message-size sweep documented
* at least one overlap study
* JSON includes collective and topology metadata

---

# v1.0 — Distributed AI-Infrastructure Performance Platform

## Objective

Turn the project into a platform for bottleneck-driven analysis of distributed AI workloads.

The system should not become a full training framework.

It should provide:

* instrumentation
* controlled experiments
* workload adapters
* evidence collection
* bottleneck analysis

## Reference Workload

Use a small but real workload:

* PyTorch DDP
* multi-GPU training
* synthetic compute/communication loop
* controlled input preprocessing

The workload exists to generate observable bottlenecks.

## Core Features

### 1. Stage-Level Instrumentation

Measure:

* input generation or loading
* CPU preprocessing
* queueing
* batching
* H2D transfer
* GPU compute
* D2H where applicable
* collective communication
* idle gaps

### 2. Bottleneck Taxonomy

Classify evidence under:

* CPU-bound
* memory-bound
* H2D-bound
* GPU launch-overhead-bound
* GPU compute-bound
* GPU memory-bound
* communication-bound
* synchronization-bound
* input-starved
* queueing/backlog-dominated

### 3. Evidence Bundle

A performance study should combine:

* end-to-end latency and throughput
* stage timing
* CPU counters
* GPU timeline evidence
* GPU kernel metrics
* communication metrics
* topology metadata

### 4. `ull-analyze`

A future tool may:

* ingest canonical JSON
* compare runs
* detect regressions
* identify likely bottleneck classes
* generate Markdown or HTML reports

The initial system should be heuristic and transparent.

It must not pretend to provide perfect automatic diagnosis.

### 5. Controlled Bottleneck Injection

Add modes that intentionally create:

* slow CPU preprocessing
* inefficient small GPU batches
* transfer serialization
* memory pressure
* communication imbalance

Purpose:

> Validate whether instrumentation can distinguish different bottleneck classes.

### 6. Scaling Study

Where hardware permits:

* 1 GPU
* 2 GPUs
* 4+ GPUs

Measure:

* throughput scaling
* scaling efficiency
* communication fraction
* idle fraction

## Flagship Performance Analysis Project A — GPU Starvation

Possible causes:

* CPU preprocessing too slow
* batch formation too slow
* H2D copies serialized
* batch too small
* synchronization gaps

The report must show evidence that distinguishes these causes.

## Flagship Project B — Distributed Scaling Breakdown

Primary question:

> Why does throughput stop scaling as GPU count increases?

Evidence:

* compute time
* communication time
* exposed communication
* synchronization
* topology

## Flagship Project C — Iterative Bottleneck Removal

```text
baseline
  -> identify dominant bottleneck
  -> optimize
  -> bottleneck shifts
  -> identify next bottleneck
  -> optimize again
```

This project should demonstrate a central principle of performance engineering:

> Removing one bottleneck usually exposes another.

## Exit Criteria

* one real distributed workload adapter
* unified stage-level result format
* CPU, GPU, and communication evidence in one report
* controlled bottleneck-injection experiment
* scaling study
* `ull-analyze` prototype or equivalent report generator

---

# Cross-Version Engineering Rules

All work from v0.4 onward must follow these rules:

1. No optimization without a stated hypothesis.
2. No performance claim without reproducible evidence.
3. Every benchmark identifies platform and build configuration.
4. Human-readable output is useful; canonical JSON is required for machine comparison.
5. Every experiment distinguishes warmup from measured samples.
6. Tail latency is reported whenever latency matters.
7. Throughput improvements are checked for latency regressions.
8. Correctness is validated before performance comparison.
9. Platform limitations and unavailable counters are documented.
10. Failed hypotheses are documented when they produce useful insight.
11. New queue or data-structure variants require a concrete performance question.
12. CPU, GPU, and distributed stages use the same analysis methodology even when tools differ.

Related documents:

* `docs/design/design_requirements.md`
* `docs/design/performance_analysis_platform.md`
* `docs/design/benchmark_result_schema.md`
