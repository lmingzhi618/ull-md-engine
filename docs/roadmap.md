# ull-md-engine Roadmap

## 1. Project Direction

`ull-md-engine` is a staged low-latency systems laboratory built around a
bounded market-data processing engine.

The project is not trying to become a production trading platform as quickly as
possible. It is also not a generic queue library, a full Disruptor clone, or a
collection of unrelated benchmarks.

The main goal is to build low-latency engineering capability through a repeated
loop:

```text
question
-> hypothesis
-> controlled implementation
-> correctness tests
-> benchmark
-> analysis
-> documented trade-off
-> milestone boundary
```

The market-data pipeline gives the experiments a coherent engineering context:

```text
input
-> decode
-> sequence / validate
-> bounded concurrent transport
-> market-state or consumer processing
-> metrics / replay / benchmark output
```

Later topics such as Linux profiling, NUMA, SIMD, DPDK, RDMA, GPU, and
AI-infrastructure performance are part of the long-term learning direction.
They should be introduced through staged prerequisites, clear learning
objectives, and bounded experiments rather than being rushed into the current
milestone.

## 2. Roadmap Philosophy

The roadmap is capability-driven rather than feature-count-driven.

Each version should answer:

1. What engineering capability is being learned?
2. What artifact demonstrates it?
3. What correctness evidence is required?
4. What benchmark evidence is required?
5. What documentation is required?
6. What is explicitly excluded?
7. What condition marks the version complete?

A version is complete when its learning objective and demonstration artifacts
are complete, not when every related idea has been explored.

## 3. Release Summary

| Version | Theme | Primary Capability | Status |
| --- | --- | --- | --- |
| v0.1 | Market Data Pipeline | Runnable measurable baseline pipeline | Complete |
| v0.2 | CPU Latency Experiments | Cache, scheduler, spin, and profiling effects | Complete enough |
| v0.3 | Concurrent Pipeline Architecture | Contention, backpressure, sequencing, and fanout | In progress |
| v0.4 | Linux Performance Laboratory | Reproducible profiling, metadata, and benchmark artifacts | Planned |
| v0.5 | Memory Determinism + NUMA | Allocation, page behavior, locality, and remote memory | Planned |
| v0.6 | CPU Hot-Path Optimization | Layout, SIMD, compiler, and branch/prefetch experiments | Planned |
| v0.7+ | Advanced I/O and Heterogeneous Performance | DPDK, RDMA, GPU, and AI-infrastructure tracks with clear prerequisites | Planned long-term |

The later stages should not be treated as committed product scope. They are
possible future research directions.

---

# v0.1 - Market Data Pipeline

## Capability

Build a runnable and measurable low-latency market-data pipeline.

## Artifact

A minimal pipeline with:

- UDP or synthetic input
- fixed-size binary message format
- SPSC transport
- consumer processing
- latency histogram
- repeatable benchmark command

## Correctness Evidence

- binary message tests
- SPSC tests
- UDP receiver tests
- basic end-to-end validation

## Benchmark Evidence

- throughput
- p50 / p99 / p999 latency
- warmup behavior
- release build command

## Documentation

- baseline docs
- module notes
- milestone status

## Non-Goals

- exchange protocol completeness
- market-state reconstruction
- multi-consumer topology
- production-grade feed handling

## Definition of Done

The baseline pipeline runs reliably, is tested, and produces stable latency
metrics.

Status: Complete.

---

# v0.2 - CPU Latency Experiments

## Capability

Isolate important sources of CPU latency and jitter.

## Artifact

A set of controlled experiments for:

- false sharing
- cache padding
- busy polling vs blocking
- thread affinity
- spin strategies
- profiling / hotspot analysis

## Correctness Evidence

- queue correctness tests continue to pass
- experiment variants preserve comparable semantics

## Benchmark Evidence

Each experiment should include:

- question
- hypothesis
- controlled variants
- command
- result table or output
- interpretation
- limitation

## Documentation

Experiment reports live under `docs/experiments/`.

Important reports include:

- `docs/experiments/false_sharing.md`
- `docs/experiments/busy_vs_blocking.md`
- `docs/experiments/thread_affinity.md`
- `docs/experiments/spin_strategies.md`

## Non-Goals

- full Linux `perf` workflow
- NUMA
- production affinity management

## Definition of Done

The main CPU latency effects are demonstrated with runnable code, measurements,
and written conclusions.

Status: Complete enough for tagged release.

---

# v0.3 - Concurrent Pipeline Architecture

## Capability

Move from low-contention SPSC behavior to contention, overload, sequencing, and
pipeline topology.

## Artifact

A bounded set of concurrency components and experiments:

- MPSC ring with per-slot sequence protocol
- blocking/spinning `push`
- non-blocking `try_push`
- drop-on-full overload mode
- producer contention benchmark
- capacity sensitivity experiment
- sequence-gap detection
- memory-ordering documentation
- single-producer sequencer exploration
- sequenced ring storage
- sequence barrier
- wait strategy
- single-producer event pipeline
- limited fanout benchmark

## Correctness Evidence

Required correctness concerns:

- producers do not overwrite unconsumed slots
- consumers do not read unpublished data
- publication makes payload visible before observation
- sequence values progress monotonically
- wrap-around remains bounded by capacity
- drop mode reports gaps explicitly
- fanout consumers observe the same published stream
- producer capacity is gated by the slowest consumer

## Benchmark Evidence

v0.3 benchmarks should show:

- producer-count scaling
- throughput
- p50 / p99 / p999 latency
- push latency where relevant
- capacity sensitivity
- overload behavior
- padded vs unpadded or cell-padded layout effects
- fanout consumer-count or affinity behavior where useful

## Documentation

Relevant documents:

- `docs/design/v0_3_concurrency_architecture.md`
- `docs/design/mpsc_memory_ordering.md`
- `docs/design/disruptor_component_map.md`
- `docs/experiments/mpsc_contention.md`
- `docs/experiments/mpsc_overload_policy.md`
- `docs/experiments/mpsc_padding.md`
- `docs/experiments/sp_pipeline_baseline.md`

## Non-Goals

v0.3 should complete the bounded Disruptor-style pipeline work already in
progress, but it should not expand without a specific learning or measurement
question.

Non-goals:

- full production LMAX Disruptor clone
- multi-producer sequencer unless a later task explicitly justifies it
- complex consumer dependency graph
- wait-strategy zoo
- general-purpose concurrency framework
- generic queue library
- unbounded topology exploration

## Definition of Done

v0.3 is complete when the bounded concurrency story is explainable end to end:

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

## Capability

Build a reproducible Linux performance-analysis foundation.

This phase exists before deeper optimization work. Its goal is reliable
evidence, not feature expansion.

## Artifact

- Linux benchmark environment notes
- strict CPU affinity support
- system topology capture
- canonical benchmark JSON output
- benchmark result metadata
- `perf stat` integration
- `perf record` / FlameGraph workflow
- comparison script for result JSON files

## Correctness Evidence

- benchmark output remains semantically equivalent across text/JSON modes where
  both exist
- JSON schema is parseable and stable
- metadata fields are documented
- benchmark scripts fail clearly on unsupported platforms

## Benchmark Evidence

At minimum collect:

- throughput
- p50 / p99 / p999
- sample count
- warmup
- capacity
- CPU placement
- context switches
- CPU migrations
- cycles
- instructions
- cache misses where available

## Documentation

- benchmark result schema
- Linux setup notes
- first `perf stat` report
- first FlameGraph-backed analysis

## Non-Goals

- CUDA
- DPDK
- RDMA
- NUMA optimization before topology support exists
- fully generic benchmark framework before repeated use justifies it

## Definition of Done

v0.4 is complete when at least one benchmark has reproducible JSON output,
Linux metadata, `perf stat` evidence, and a documented analysis workflow.

Status: Planned / beginning through canonical JSON work.

---

# v0.5 - Memory Determinism + NUMA

## Capability

Understand how allocation, page behavior, cache/TLB effects, and memory locality
affect latency and throughput.

## Artifact

Potential experiments:

- hot-path no-allocation validation
- allocator jitter comparison
- preallocation / pool baseline
- page-fault experiment
- pre-touch experiment
- huge-page note where practical
- NUMA local vs remote memory comparison on suitable hardware

## Non-Goals

- allocator framework without a current pipeline need
- NUMA work on hardware that cannot demonstrate NUMA behavior
- memory tricks without benchmark evidence

## Definition of Done

One or more memory experiments produce correctness evidence, latency evidence,
and a written explanation of the mechanism.

Status: Planned.

---

# v0.6 - CPU Hot-Path Optimization

## Capability

Optimize a real pipeline hot path and explain the improvement with
microarchitectural evidence.

## Artifact

Potential experiments:

- scalar baseline
- data-layout comparison
- SIMD implementation where hardware supports it
- compiler flag comparison
- assembly inspection where useful
- branch or prefetch experiment when driven by evidence

## Non-Goals

- toy SIMD unrelated to the pipeline
- keeping optimizations that do not measure better
- architecture-specific code without a scalar reference

## Definition of Done

A profile-to-optimization case study demonstrates correctness, benchmark
improvement, counter evidence, and trade-offs.

Status: Planned.

---

# Planned Long-Term Research Tracks

The following tracks are part of the long-term learning plan, not discarded or
optional trivia:

- DPDK and kernel-bypass networking
- RDMA and low-latency network transport
- GPU acceleration
- heterogeneous CPU/GPU pipelines
- distributed AI-infrastructure performance analysis

They should not interrupt the current v0.3/v0.4 sequence. Each track requires
prerequisites, hardware or environment support, a design note, and a bounded
experiment before implementation.

A likely long-term ordering is:

```text
v0.4 Linux performance foundation
-> v0.5 memory determinism and NUMA
-> v0.6 CPU hot-path optimization
-> v0.7 kernel-bypass networking: DPDK / RDMA
-> v0.8 GPU acceleration
-> v0.9 heterogeneous pipeline performance
-> v1.0 AI-infrastructure performance studies
```

Some advanced tracks may still deserve separate repositories if they grow beyond
the bounded market-data pipeline context.

---

## Immediate Next Steps

Before choosing the next task, inspect the repository state and compare code,
tests, benchmarks, and documentation.

Current likely direction:

1. Finish the bounded v0.3 Disruptor-style sequencer/fanout pipeline.
2. Stabilize canonical JSON output for benchmark results.
3. Update project status after each bounded milestone.
4. Begin v0.4 through a small Linux profiling or metadata experiment.

Do not jump directly to CUDA, DPDK, RDMA, or distributed AI implementation from
the current state. Keep them visible as planned long-term tracks.
