# SRS v0.2 — ULL Market Data Engine

## 1. Purpose
v0.2 extends the v0.1 MVP into a more realistic market-data pipeline benchmark.

The goal is to preserve a simple SPSC architecture while improving:
- message realism
- burst traffic simulation
- latency statistics
- benchmark usability

## 2. Scope
v0.2 includes:
- a market-data-like message structure (`MdTick`)
- producer/consumer benchmark over SPSC queue
- latency measurement from producer timestamp to consumer receive time
- summary statistics: min / max / avg / p50 / p99
- unit tests for core components

v0.2 excludes:
- networking
- order book construction
- MPMC queues
- persistence
- advanced CPU affinity / NUMA tuning

## 3. Functional Requirements

### FR-1 Message Model
The system shall define a fixed-size market data tick message containing:
- sequence number
- producer timestamp
- instrument identifier
- channel identifier
- bid/ask price
- bid/ask quantity

### FR-2 Queue Transport
The system shall transport messages through a bounded SPSC ring buffer.

### FR-3 Producer
The producer shall publish a configurable number of messages.
The producer may publish messages in bursts.

### FR-4 Consumer
The consumer shall continuously drain the queue and compute per-message latency.

### FR-5 Statistics
The system shall report:
- total message count
- min latency
- max latency
- average latency
- p50 latency
- p99 latency
- approximate throughput

## 4. Non-Functional Requirements

### NFR-1 Simplicity
The implementation should remain small and understandable.

### NFR-2 Determinism
Core logic should be testable with unit tests.

### NFR-3 Performance Orientation
The implementation should avoid unnecessary dynamic allocation in the hot path.

## 5. Acceptance Criteria
v0.2 is complete when:
- all tests pass
- the benchmark executable runs successfully
- benchmark output includes latency summary
- README documents build and run instructions
