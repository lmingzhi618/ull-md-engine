# Global Design Requirements

## 1. Purpose

This document defines cross-version engineering requirements for `ull-md-engine`.

These requirements apply to both:

* human-authored changes
* Codex-assisted implementation

Version-specific design documents may add stricter requirements.

They should not silently weaken these requirements.

The project is a performance-engineering platform, not a collection of isolated benchmarks.

New work should strengthen the ability to:

```text
form a performance question
  -> state a hypothesis
  -> create a controlled experiment
  -> collect reproducible evidence
  -> identify a bottleneck
  -> implement an optimization
  -> validate the result
  -> explain the trade-off
```

---

## 2. Architecture Requirements

### DR-1 — Preserve Staged Evolution

New work should fit the roadmap:

```text
market data
  -> CPU performance
  -> concurrency
  -> Linux profiling
  -> memory and NUMA
  -> SIMD
  -> GPU
  -> heterogeneous pipelines
  -> multi-GPU communication
  -> distributed AI infrastructure
```

Do not add a technology only because it is popular.

Every major component must support:

* a roadmap milestone
* a documented performance question
* or both

---

### DR-2 — Separate Workloads from Measurement Infrastructure

Prefer logical boundaries between:

```text
workload
instrumentation
result model
artifact writer
analysis
```

Benchmark executables should not independently reimplement:

* JSON formatting
* system metadata capture
* percentile serialization
* result comparison
* artifact indexing

when reusable infrastructure already exists.

---

### DR-3 — Separate End-to-End and Stage Timing

Preserve both:

* user-observed end-to-end latency
* internal per-stage latency

Examples of future stages:

```text
input
CPU preprocessing
queue wait
batch formation
H2D
GPU kernel
D2H
collective communication
```

Per-stage timing must not replace end-to-end timing.

---

### DR-4 — Keep Reference Implementations

Optimized implementations should retain a simple reference path where practical.

Examples:

* scalar vs SIMD
* CPU vs GPU
* pageable vs pinned memory
* synchronous vs asynchronous copy
* baseline queue vs optimized queue

Correctness must be comparable before performance is compared.

---

### DR-5 — Avoid Premature Framework Construction

Do not create a generalized framework before at least two real use cases justify the abstraction.

This applies especially to:

* profiler adapters
* GPU backends
* workload interfaces
* analysis plug-ins
* generic benchmark frameworks

Small and explicit code is preferred over speculative abstraction.

---

## 3. Performance Experiment Requirements

### DR-6 — Every Experiment Starts with a Question

Every experiment document must state:

1. Question
2. Hypothesis
3. Independent variable
4. Controlled variables
5. Metrics
6. Run procedure
7. Results
8. Interpretation
9. Limitations
10. Follow-up

---

### DR-7 — Baseline Before Optimization

No optimization result is valid without a baseline produced under:

* the same environment
* the same workload
* the same measurement method
* equivalent instrumentation

---

### DR-8 — One Major Variable per Experiment

When possible, change one major factor at a time.

If multiple factors change together, the report must explicitly state that causal attribution is limited.

---

### DR-9 — Warmup Must Be Explicit

Benchmarks must distinguish:

```text
setup
warmup
measurement
teardown
```

Canonical JSON must record:

* warmup policy
* warmup operations or duration
* measured operations
* measured sample count

---

### DR-10 — Repetition Is First-Class

Performance experiments should support repeated trials.

Where practical, report:

* individual trial values
* aggregate values
* variability

Do not claim an improvement based on one noisy run.

---

### DR-11 — Tail Latency Is Required

For latency-sensitive paths, report at least:

* p50
* p99
* p999 when sample count is sufficient
* max

Mean latency alone is insufficient.

---

### DR-12 — Throughput Changes Require Latency Checks

A throughput optimization may increase:

* queue residency
* batch delay
* synchronization delay
* p99
* p999

Therefore throughput claims must be checked against latency when latency matters.

---

### DR-13 — Instrumentation Overhead Must Be Considered

Timing and tracing can perturb the system.

Experiment documentation should state:

* what instrumentation was enabled
* whether all variants used identical instrumentation
* known or estimated measurement overhead

---

## 4. Canonical Result Requirements

### DR-14 — Canonical JSON Output

All new benchmark executables from v0.4 onward must support JSON output conforming to:

```text
docs/design/benchmark_result_schema.md
```

Human-readable console output may remain.

JSON is the canonical machine-readable artifact.

---

### DR-15 — Required Run Metadata

Results should record, when available:

* schema version
* benchmark name
* benchmark version
* git commit
* dirty-tree flag
* UTC timestamp
* build type
* compiler
* compiler version
* compiler flags
* OS
* kernel
* host identifier
* CPU model
* CPU topology
* NUMA topology
* GPU model
* driver version
* CUDA version
* benchmark arguments
* warmup policy
* measured sample count

Unavailable values should be:

* omitted
* or explicitly `null`

Never invent metadata.

---

### DR-16 — Stable Units

Use explicit units.

Preferred canonical units:

* time: nanoseconds
* size: bytes
* bandwidth: bytes/second
* throughput: operations/second or messages/second
* frequency: Hz
* percentage: numeric 0–100

Do not mix:

* cycles
* ticks
* nanoseconds

under one field name.

---

### DR-17 — Raw Artifacts Must Be Linkable

A benchmark result may reference:

* `perf stat` output
* `perf.data`
* FlameGraph SVG
* Nsight Systems report
* Nsight Compute report
* NCCL logs
* topology output

Artifact paths should preferably be relative to the run directory.

---

## 5. Linux and CPU Requirements

### DR-18 — Linux Is the Reference Performance Platform from v0.4

macOS remains useful for development.

However, serious claims involving:

* strict CPU affinity
* hardware counters
* NUMA
* page policy
* Linux scheduling behavior

must be produced on Linux.

---

### DR-19 — CPU Placement Must Be Explicit

When placement affects the experiment, benchmark configuration should support explicit CPU assignment.

Result JSON should record:

* requested CPU set
* placement policy
* actual placement when observable

---

### DR-20 — Hardware Counter Collection Is Additive

Do not embed expensive PMU collection into hot-path application code unless a specific experiment requires it.

Prefer:

* external tooling
* wrapper scripts
* isolated collectors

---

### DR-21 — Counter Availability Is Platform-Specific

A benchmark should not fail solely because an optional hardware counter is unavailable.

Record:

* requested event
* availability
* measured value
* or unavailable status

---

## 6. Memory and NUMA Requirements

### DR-22 — Memory Policy Is Experiment Configuration

When relevant, record:

* allocator
* preallocation policy
* pre-touch policy
* page mode
* memory binding
* NUMA node

---

### DR-23 — Hot-Path Allocation Claims Must Be Verifiable

A claim such as:

> no allocation on the hot path

should be backed by at least one of:

* allocator instrumentation
* test hook
* profiler evidence
* explicit implementation invariant with tests

---

### DR-24 — NUMA Experiments Require Topology Evidence

A local-vs-remote memory result is invalid without recorded:

* CPU placement
* memory placement
* NUMA topology

---

## 7. GPU Requirements

### DR-25 — Separate Host and Device Timing

GPU workloads should capture:

* host end-to-end time
* queue wait
* batch wait
* H2D time
* kernel time
* D2H time where applicable

Use GPU-native events for device timing.

---

### DR-26 — GPU Metadata Is Required

Record:

* GPU model
* device count
* device IDs
* driver version
* CUDA version
* kernel launch configuration
* stream count
* memory mode
* batch size

---

### DR-27 — GPU Optimization Must Be Pipeline-Aware

A faster GPU kernel may not improve end-to-end performance when the system is:

* CPU-bound
* input-starved
* transfer-bound
* serialized
* communication-bound

At least one v0.8 study must use timeline evidence.

---

## 8. Multi-GPU and Distributed Requirements

### DR-28 — Topology Is Part of the Result

Multi-GPU results should capture:

* device IDs
* GPU topology
* NUMA relationship
* interconnect type when known

---

### DR-29 — Distinguish Total and Exposed Communication

When communication/computation overlap exists, measure separately:

* total communication duration
* exposed communication duration
* hidden communication duration
* overlap ratio

---

### DR-30 — Scaling Requires an Efficiency Metric

A multi-device result should not report throughput alone.

Report where meaningful:

```text
scaling_efficiency =
observed_speedup / ideal_speedup
```

---

## 9. Documentation Requirements

### DR-31 — Every Major Experiment Gets a Document

Use:

```text
docs/experiments/
```

A major performance claim should have a corresponding experiment document.

---

### DR-32 — Failed Hypotheses May Be Valuable Results

Do not delete an experiment merely because:

* an optimization did not help
* a hypothesis was wrong
* results were platform-specific

Document useful negative findings.

Examples:

```text
manual prefetch did not help
larger queue increased latency
padding removed false sharing but not atomic contention
```

These are valuable engineering results.

---

### DR-33 — Separate Observation from Interpretation

Experiment documents should clearly distinguish:

```text
Measured:
p99 increased by 32%.

Interpretation:
The increase is consistent with increased queue residency.
```

Do not present interpretation as direct measurement.

---

## 10. Codex Implementation Requirements

Before implementing a roadmap task, Codex should read:

1. `docs/design/design_requirements.md`
2. `docs/roadmap.md`
3. the relevant version-specific design document
4. related experiment documents

Codex should:

* preserve existing architecture unless change is justified
* avoid unrelated refactors
* add tests before trusting benchmark results
* preserve baseline variants
* emit canonical JSON for new v0.4+ benchmarks
* update documentation when an experiment changes
* avoid claiming performance improvements without measured results
* avoid filling result documents with hypothetical numbers

Codex should not:

* fabricate benchmark results
* fabricate hardware metadata
* claim performance improvements based only on code inspection
* add queue variants without a performance question
* over-generalize infrastructure before real usage exists

---

## 11. Performance Change Review Checklist

Before a performance feature is considered complete:

### Correctness

* Does the baseline pass?
* Does the optimized path pass?
* Are reference and optimized outputs equivalent?

### Experiment Design

* Is the question explicit?
* Is the hypothesis explicit?
* Are controlled variables documented?

### Reproducibility

* Are build details recorded?
* Is platform metadata recorded?
* Is the run command documented?

### Measurement

* Is warmup separated?
* Are units explicit?
* Is tail latency reported where relevant?
* Is throughput reported where relevant?

### Evidence

* Are before and after results available?
* Are profiler or counter artifacts linked when relevant?

### Interpretation

* Is the bottleneck explanation supported by evidence?
* Are limitations documented?
* Is the next likely bottleneck identified?
