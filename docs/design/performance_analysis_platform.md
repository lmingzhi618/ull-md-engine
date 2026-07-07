# Performance Analysis Platform Design

## 1. Purpose

This document defines the architecture and workflow for performance analysis in `ull-md-engine`.

The platform exists to answer performance questions through evidence.

It should support the same core methodology across:

* low-latency CPU pipelines
* concurrent queues
* Linux scheduler and PMU analysis
* memory hierarchy and NUMA
* SIMD optimization
* CUDA workloads
* heterogeneous CPU/GPU pipelines
* multi-GPU communication
* distributed AI infrastructure

The platform is not intended to be a generic benchmarking framework for every possible workload.

It should evolve incrementally from real project needs.

---

## 2. Core Performance Methodology

Every major performance analysis project should follow:

```text
question
  -> hypothesis
  -> experiment design
  -> baseline
  -> measurement
  -> evidence collection
  -> bottleneck attribution
  -> optimization
  -> validation
  -> new bottleneck identification
```

The strongest project narrative is not:

```text
I changed the code and it became faster.
```

It is:

```text
The workload showed symptom X.

Evidence A and B suggested bottleneck Y.

I changed Z to address Y.

Metric M improved.

Metric N did not improve.

The remaining evidence suggests bottleneck Q is now dominant.
```

---

# 3. Conceptual Architecture

The platform should gradually separate five concerns:

```text
┌─────────────────────────────────────┐
│              Workload               │
│ market data / synthetic / AI        │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│          Instrumentation            │
│ latency / stages / counters / GPU   │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│            Result Model             │
│ canonical benchmark result          │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│          Artifact Collection        │
│ JSON / perf / FlameGraph / Nsight   │
└──────────────────┬──────────────────┘
                   │
┌──────────────────▼──────────────────┐
│              Analysis               │
│ compare / visualize / diagnose      │
└─────────────────────────────────────┘
```

These boundaries are conceptual.

They should not force premature class hierarchies or framework abstractions.

---

# 4. Workload Layer

## 4.1 Responsibility

The workload layer creates useful system behavior.

Examples:

### Existing workloads

* SPSC producer/consumer pipeline
* UDP market-data pipeline
* MPSC contention benchmark
* busy-vs-blocking benchmark
* spin-strategy benchmark

### Future workloads

* binary replay pipeline
* allocation stress workload
* SIMD parsing workload
* CPU/GPU transform
* multi-stage heterogeneous pipeline
* NCCL collective workload
* PyTorch DDP workload

A workload should expose enough configuration to create controlled experiments.

---

## 4.2 Workload Requirements

Each benchmark workload should define:

* benchmark name
* workload version
* configuration
* warmup policy
* measurement policy
* completion condition
* relevant correctness criteria

The workload should not directly own all analysis infrastructure.

For example, future benchmark code should avoid separately implementing:

```text
system metadata collection
JSON escaping
git metadata collection
result directory naming
comparison logic
```

in every executable.

---

# 5. Instrumentation Layers

Instrumentation should be layered because different tools answer different questions.

---

## 5.1 Layer 1 — End-to-End Metrics

These represent externally observable behavior.

Examples:

* end-to-end latency
* operations per second
* messages per second
* total runtime
* success count
* drop count
* sequence gaps

For latency-sensitive workloads, report:

* minimum
* p50
* p99
* p999
* maximum

Average may also be reported, but should not replace percentiles.

---

## 5.2 Layer 2 — Application Stage Timing

Stage timing explains where end-to-end time is spent.

Current and future examples:

```text
UDP receive
queue push
queue residency
queue pop
CPU preprocessing
batch wait
H2D
GPU kernel
D2H
collective communication
synchronization wait
```

Stage timings should allow analysis such as:

```text
end-to-end latency increased

but

kernel time remained unchanged

and

queue residency increased
```

This helps separate:

* slow computation
* waiting
* backlog
* synchronization
* transfer

---

## 5.3 Layer 3 — Operating-System Evidence

Examples:

* context switches
* CPU migrations
* page faults
* scheduler behavior
* CPU placement
* memory placement

Primary tools may include:

* `perf stat`
* `perf record`
* `taskset`
* `numactl`
* `/proc`
* `/sys`

This layer becomes central in v0.4 and v0.5.

---

## 5.4 Layer 4 — CPU Microarchitectural Evidence

Examples:

* cycles
* instructions
* IPC
* branch misses
* cache misses
* LLC misses
* TLB misses
* stall events

A microarchitectural metric should not be interpreted in isolation.

For example:

```text
cache misses decreased
```

does not automatically prove:

```text
the optimization succeeded because of fewer cache misses
```

The analysis should consider:

* latency
* throughput
* cycles
* instructions
* workload equivalence
* measurement noise

---

## 5.5 Layer 5 — GPU Timeline Evidence

For heterogeneous workloads, timeline evidence should answer:

* Is the GPU idle?
* Is the CPU late?
* Are transfers serialized?
* Are kernel launches separated by gaps?
* Does copy overlap with compute?
* Does one stage block the next?

Primary tool:

* Nsight Systems

This layer becomes central in v0.8.

---

## 5.6 Layer 6 — GPU Kernel Evidence

Kernel analysis should answer:

* Is the kernel compute-bound?
* Is it memory-bound?
* Is occupancy limiting execution?
* Which warp stalls dominate?
* Is memory access efficient?

Primary tool:

* Nsight Compute

This layer should be used selectively.

Do not profile every kernel deeply without a reason.

---

## 5.7 Layer 7 — Communication Evidence

For multi-GPU and distributed workloads:

* collective latency
* algorithm bandwidth
* bus bandwidth
* communication fraction
* exposed communication
* hidden communication
* topology
* scaling efficiency

This layer becomes central in v0.9 and v1.0.

---

# 6. Benchmark Execution Model

A benchmark run should conceptually follow:

```text
initialize
  -> validate configuration
  -> capture static environment
  -> allocate resources
  -> warm up
  -> begin measured region
  -> run workload
  -> stop measured region
  -> collect metrics
  -> write canonical result
  -> preserve external artifacts
```

---

## 6.1 Setup Phase

Setup may include:

* parsing CLI arguments
* allocating queues
* allocating memory
* selecting CPUs
* selecting GPUs
* loading replay data
* initializing CUDA
* initializing communicators

Setup should not normally contaminate steady-state measurements.

---

## 6.2 Warmup Phase

Warmup may be defined by:

* operation count
* duration
* iteration count

Warmup policy must be recorded.

Examples:

```text
warmup_messages = 100000
```

or:

```text
warmup_duration_ns = 5000000000
```

---

## 6.3 Measurement Phase

The measurement phase should define:

* start condition
* stop condition
* number of measured samples
* instrumentation enabled

All comparison variants should use equivalent measurement boundaries.

---

## 6.4 Cooldown and Finalization

Some future workloads may require:

* CUDA synchronization
* communication synchronization
* background-thread shutdown
* delayed profiler flush

The benchmark should not write incomplete results before asynchronous work completes.

---

# 7. Run Directory Design

From v0.4 onward, a performance run may produce a structured artifact directory.

Recommended pattern:

```text
results/
└── <benchmark-name>/
    └── <timestamp>-<short-id>/
        ├── result.json
        ├── console.txt
        ├── command.txt
        ├── perf-stat.txt
        ├── profile/
        │   ├── perf.data
        │   └── flamegraph.svg
        ├── gpu/
        │   ├── nsys-report.nsys-rep
        │   └── ncu-report.ncu-rep
        └── topology/
            ├── cpu.txt
            ├── numa.txt
            └── gpu.txt
```

Not every run needs every artifact.

The result JSON should reference available artifacts.

---

# 8. Benchmark Identity

Each result needs stable identity fields.

Conceptually:

```text
benchmark name
benchmark version
result schema version
git commit
configuration
platform
timestamp
```

Two runs should be comparable only when the comparison is meaningful.

The comparison tool should eventually detect obvious incompatibilities such as:

```text
different benchmark
different workload version
different units
different message size
```

and warn the user.

---

# 9. Performance Analysis Project Structure

Each major project should have:

```text
docs/experiments/<project-name>.md
```

Recommended document structure:

```text
# Title

## 1. Question

## 2. Motivation

## 3. Hypothesis

## 4. System Under Test

## 5. Variables

## 6. Metrics

## 7. Environment

## 8. Procedure

## 9. Results

## 10. Analysis

## 11. Limitations

## 12. Conclusion

## 13. Next Questions
```

---

# 10. Evidence Hierarchy

Not all evidence has equal strength.

A useful hierarchy is:

```text
Level 1:
observed latency or throughput change

Level 2:
stage timing identifies where time changed

Level 3:
OS or hardware counters support a mechanism

Level 4:
profiler timeline or call stack locates the behavior

Level 5:
controlled intervention changes the predicted metric
```

Example:

```text
Observation:
p99 improved.

Better evidence:
queue residency decreased.

Stronger evidence:
queue capacity reduction decreased residency
without changing consumer service time.

Strongest evidence:
restoring large capacity reproduced the higher residency.
```

The project should aim for stronger causal evidence when practical.

---

# 11. Bottleneck Taxonomy

The analysis platform should use a shared vocabulary.

---

## 11.1 Queueing / Backlog Bound

Symptoms:

* growing queue depth
* increased residency time
* stable consumer service time
* burst-amplified tail latency

Possible causes:

* producer rate exceeds consumer rate
* oversized buffering
* downstream stall

---

## 11.2 Synchronization / Contention Bound

Symptoms:

* atomic hotspot
* CAS retries
* shared-cache-line traffic
* throughput collapse with producer count
* increased producer latency

Possible causes:

* shared head
* lock contention
* cache-line ownership transfer

---

## 11.3 Scheduler Bound

Symptoms:

* context switches
* migrations
* wakeup latency
* high variance
* improved latency after pinning

---

## 11.4 Memory-Latency Bound

Symptoms:

* poor locality
* LLC misses
* NUMA penalty
* pointer chasing
* low useful instruction rate

---

## 11.5 TLB / Page Bound

Symptoms:

* dTLB misses
* page faults
* large working set
* improvement from pre-touch or huge pages

---

## 11.6 Branch Bound

Symptoms:

* high branch-miss rate
* data-dependent control flow
* improvement from layout or branch reduction

---

## 11.7 CPU Compute Bound

Symptoms:

* CPU fully occupied
* little waiting
* high instruction work per item
* performance improves with SIMD or algorithm changes

---

## 11.8 GPU Launch-Overhead Bound

Symptoms:

* tiny kernels
* large gaps relative to kernel duration
* poor efficiency at small batch sizes

---

## 11.9 Host-to-Device Transfer Bound

Symptoms:

* H2D dominates stage time
* GPU idle during transfer
* improvement from pinned memory or overlap

---

## 11.10 GPU Compute Bound

Symptoms:

* high GPU utilization
* compute dominates total time
* communication and input pipeline are not dominant

---

## 11.11 GPU Memory Bound

Symptoms:

* memory throughput near useful limits
* low arithmetic intensity
* memory-related warp stalls

---

## 11.12 Input-Starved

Symptoms:

* GPU idle
* slow CPU preprocessing
* long batch-generation gaps
* kernel itself is efficient

---

## 11.13 Communication Bound

Symptoms:

* collective time dominates
* poor multi-GPU scaling
* large exposed communication fraction

---

## 11.14 Topology Bound

Symptoms:

* large differences by device placement
* cross-NUMA penalty
* PCIe path penalty
* reduced bandwidth under unfavorable topology

---

## 11.15 Synchronization Bound in Distributed Workloads

Symptoms:

* devices finish useful work at different times
* barrier gaps
* one slow rank delays all others

---

# 12. Performance Comparison Model

The analysis tooling should eventually compare two or more result files.

Minimum comparison output:

```text
baseline
candidate
absolute difference
percentage difference
```

For example:

```text
p99_ns:
baseline:   1200
candidate:   850
delta:      -350
change:   -29.2%
```

---

## 12.1 Comparison Categories

### Improvement

Candidate improves the target metric without violating key constraints.

### Regression

Candidate degrades a target metric.

### Trade-Off

Example:

```text
throughput +25%
p99 latency +40%
```

The tool should not label this simply as an improvement.

### Inconclusive

Use when:

* variation is large
* insufficient repetitions exist
* configurations differ materially
* instrumentation differs

---

# 13. Statistical Scope

This project does not initially need a sophisticated statistical framework.

However, performance conclusions should consider:

* repeated runs
* variance
* outliers
* sample count
* environmental noise

Recommended evolution:

### Initial

* repeated trials
* median across trials
* min/max across trials

### Later

Potentially add:

* standard deviation
* coefficient of variation
* confidence intervals

Only add statistical machinery when real experiments justify it.

---

# 14. Analysis Tooling Roadmap

## Stage 1 — Result Loader

Provide:

```text
load result.json
validate schema version
display summary
```

---

## Stage 2 — Compare

Provide:

```text
ull-analyze compare baseline.json candidate.json
```

Output:

* metadata differences
* metric differences
* obvious regressions
* warnings

---

## Stage 3 — Plot

Generate:

* latency comparison
* throughput comparison
* batch-size sweep
* producer-count scaling
* message-size sweep

---

## Stage 4 — Evidence Correlation

Example output:

```text
Observed:
p99 latency decreased 31%.

Related changes:
CPU migrations: -95%
context switches: -71%
IPC: +4%

Interpretation:
The result is consistent with reduced scheduler interference.
```

The tool must label this as an interpretation.

---

## Stage 5 — Bottleneck Heuristics

Future `ull-analyze` may report:

```text
Likely bottleneck class:
input-starved

Evidence:
GPU active time: 42%
CPU preprocessing: 79% of pipeline stage time
large gaps before H2D submission
kernel occupancy appears healthy
```

Heuristics must:

* show evidence
* expose thresholds
* avoid pretending to be definitive

---

# 15. Flagship Performance Analysis Projects

The project should eventually contain several complete, interview-quality studies.

---

## Project 1 — False Sharing

Question:

> How much can cache-line ownership contention affect a two-thread workload?

Evidence:

* padded vs unpadded
* runtime
* latency
* hardware counters where later available

Status:

Existing foundation.

---

## Project 2 — Scheduler and Affinity

Question:

> How do migration, SMT placement, and physical-core placement affect tail latency?

Required:

* Linux affinity
* perf counters
* latency distributions

Target release:

v0.4.

---

## Project 3 — Hot-Path Root-Cause Analysis

Question:

> Which code path dominates the benchmark, and can profiler evidence guide a measurable optimization?

Required:

* perf stat
* perf record
* FlameGraph
* before/after

Target release:

v0.4.

---

## Project 4 — Allocation Tail Jitter

Question:

> Why can allocator behavior strongly affect extreme latency while barely changing average throughput?

Required:

* allocator variants
* p99/p999/max
* page-fault evidence

Target release:

v0.5.

---

## Project 5 — Local vs Remote NUMA Memory

Question:

> How much performance is lost when compute and memory placement diverge?

Required:

* topology
* CPU affinity
* memory binding
* latency
* throughput
* counters

Target release:

v0.5.

---

## Project 6 — Scalar to SIMD

Question:

> Can a real hot path be accelerated through data layout and vectorization?

Required:

* scalar baseline
* profile
* layout analysis
* SIMD path
* cycles/item
* assembly evidence where useful

Target release:

v0.6.

---

## Project 7 — CPU/GPU Crossover

Question:

> At what workload size does GPU execution overcome launch and transfer overhead?

Required:

* identical logical operation
* CPU path
* GPU path
* size sweep

Target release:

v0.7.

---

## Project 8 — Batch-Size Phase Diagram

Question:

> How does the dominant bottleneck change as batch size increases?

Expected regions:

```text
tiny batch
  -> launch overhead

small batch
  -> low utilization

medium batch
  -> efficient GPU execution

large batch
  -> queueing and latency growth
```

Target release:

v0.7.

---

## Project 9 — CPU/GPU Pipeline Overlap

Question:

> How much serialized copy and compute time can be hidden through pinned memory, asynchronous transfer, and streams?

Required:

* Nsight Systems baseline
* optimization sequence
* overlap calculation

Target release:

v0.8.

---

## Project 10 — GPU Starvation Diagnosis

Question:

> Why is the GPU idle?

Possible root causes:

* CPU preprocessing
* batch construction
* transfer
* launch gap
* synchronization

Target release:

v0.8 or v1.0.

---

## Project 11 — Collective Performance Curve

Question:

> How do latency and bandwidth behavior change with collective type and message size?

Required:

* NCCL
* message-size sweep
* topology

Target release:

v0.9.

---

## Project 12 — Distributed Scaling Breakdown

Question:

> Why does throughput stop scaling as GPU count increases?

Required evidence:

* compute
* communication
* exposed communication
* idle time
* synchronization
* topology

Target release:

v1.0.

---

# 16. Anti-Patterns

Avoid:

## Benchmark Collection Without Questions

Bad:

```text
I benchmarked 20 queues.
```

Better:

```text
I tested whether shared producer reservation becomes dominant
as producer count increases.
```

---

## Optimization by Folklore

Bad:

```text
Prefetch should be faster.
```

Better:

```text
The profile indicates memory stalls on a predictable access stream.
The hypothesis is that prefetching distance D may reduce exposed latency.
```

---

## Single-Metric Optimization

Bad:

```text
Throughput improved by 20%.
```

Better:

```text
Throughput improved by 20%, but p999 increased by 35%
because batching increased queue residency.
```

---

## Profiler Screenshot Without Analysis

A FlameGraph or Nsight screenshot is not a conclusion.

The report must explain:

* what is visible
* what hypothesis it supports
* what intervention followed
* what changed

---

## Automatic Diagnosis Without Evidence

The future analysis tool must never state:

```text
The bottleneck is definitely X.
```

when the available data only supports:

```text
X is the leading hypothesis.
```

---

# 17. Success Criteria

The performance-analysis platform is successful when the project can repeatedly demonstrate:

```text
observe symptom
  -> locate bottleneck
  -> explain mechanism
  -> change system
  -> validate result
  -> identify next limit
```

The long-term portfolio should prove not only that the author can write fast code, but that the author can diagnose and optimize complex systems across CPU, GPU, and distributed infrastructure.
