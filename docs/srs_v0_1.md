# SRS v0.1 — ULL MD Engine MVP

## 1. Purpose
v0.1 defines the minimum viable pipeline for the ULL MD Engine project.

The goal of this version is to establish a small, correct, and measurable
single-producer/single-consumer market-data path that can serve as the baseline
for later performance engineering work.

## 2. Scope
v0.1 includes:
- a bounded SPSC ring buffer
- a synthetic message producer
- a consumer that drains messages from the ring
- latency measurement from producer timestamp to consumer receive time
- a benchmark/demo executable
- unit tests for the core queue behavior

v0.1 excludes:
- CPU affinity tuning
- replay input mode
- flamegraph / perf profiling integration
- MPSC or fanout topologies
- custom allocators
- NUMA-aware placement

## 3. Functional Requirements

### FR-1 Queue
The system shall provide a bounded single-producer/single-consumer ring buffer.

### FR-2 Producer
The system shall generate synthetic feed messages and push them into the ring.

### FR-3 Consumer
The system shall continuously drain the ring and process all produced messages.

### FR-4 Latency Measurement
The system shall measure end-to-end latency from producer timestamp to consumer
receive timestamp.

### FR-5 Benchmark Harness
The system shall provide an executable that runs the producer/consumer pipeline
and prints a latency summary.

## 4. Non-Functional Requirements

### NFR-1 Simplicity
The implementation should remain small and easy to understand.

### NFR-2 Correctness
Core queue semantics shall be covered by unit tests.

### NFR-3 Stable Baseline
The implementation should be suitable as a baseline for later engineering
experiments in v0.2 and beyond.

## 5. Acceptance Criteria
v0.1 is complete when:
- all unit tests pass
- the benchmark executable runs successfully
- the pipeline transports all produced messages to the consumer
- benchmark output includes latency statistics
- README documents build and run instructions
