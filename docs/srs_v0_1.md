# SRS v0.1 — ULL Market Data Engine (MVP)

## 1. Purpose
This document specifies the requirements for v0.1 (MVP) of the ULL Market Data Engine project.  
The MVP focuses on establishing a correct, testable, and unit-consistent low-latency pipeline:

**Producer → SPSC Queue → Consumer → Latency Measurement/Reporting**

The MVP is intentionally minimal: it is not a full trading system. It is a foundation for later engineering-grade iterations.

## 2. Scope

### 2.1 In Scope (v0.1)
1. **Monotonic tick source**
   - Provide `ticks()` for monotonic measurement.
   - Provide `ticks_to_ns(delta)` so all reported latencies have a stable unit (nanoseconds).
2. **SPSC ring buffer**
   - Fixed-capacity ring buffer supporting single-producer single-consumer.
   - Non-blocking operations: `try_push`, `try_pop`.
3. **Latency histogram**
   - Record latency samples in **nanoseconds**.
   - Provide percentile summary output: p50, p99, p999.
4. **Benchmark harness (integration)**
   - A simple producer/consumer program that drives the queue and records latency.
   - Parameters: message count `N`, warmup count `WARMUP`.
5. **Unit testing**
   - Each module must have unit tests (ticks, ring buffer, histogram).
   - Tests must run in CI.

### 2.2 Out of Scope (explicit non-goals for v0.1)
- Real market protocols (ITCH/FIX/OUCH), real exchange connectivity
- Limit order book / matching engine logic
- CPU affinity / thread pinning / real-time priorities
- NUMA-aware design
- Custom allocators / object pools
- Kernel-bypass networking (DPDK, io_uring, etc.)
- Full integration/performance validation across platforms

## 3. Definitions
- **Tick**: a monotonic counter used for measuring time intervals.
- **ns**: nanoseconds.
- **SPSC**: single producer, single consumer.

## 4. Functional Requirements

### FR-1 Tick API
- The system SHALL provide:
  - `init_ticks()` to perform any one-time initialization/calibration.
  - `ticks()` to return a monotonic tick.
  - `ticks_to_ns(delta)` to convert a tick delta into nanoseconds.
- All latency values stored in histograms and printed in reports SHALL be in nanoseconds.

### FR-2 SPSC Ring Buffer API
- The system SHALL provide a fixed-capacity ring buffer:
  - `try_push(const T&) -> bool`: returns false when full.
  - `try_pop(T&) -> bool`: returns false when empty.
- The ring buffer SHALL be FIFO for successfully enqueued items.
- The ring buffer SHALL NOT allocate memory after construction.

### FR-3 Latency Histogram API
- The histogram SHALL:
  - Accept latency samples in nanoseconds via `add(ns)`.
  - Cap input values greater than `max_ns` to `max_ns` (no out-of-bounds behavior).
  - Report `count`, `p50_ns`, `p99_ns`, `p999_ns` in a deterministic format.

### FR-4 Benchmark Harness
- The benchmark program SHALL:
  - Parse `N` and `WARMUP` from command line.
  - Run a producer that enqueues `N` messages, each including a send tick.
  - Run a consumer that dequeues messages and records `(ticks_now - t_send)` converted to ns.
  - Discard the first `WARMUP` observations (warm-up phase).
  - Print a report including config and histogram summary.

## 5. Non-Functional Requirements

### NFR-1 Unit Consistency
- All reported latency metrics SHALL be in nanoseconds.

### NFR-2 Determinism of Output Format
- The report output format SHALL remain stable across runs (values differ; labels and structure do not).

### NFR-3 Testability
- Each module SHALL have unit tests covering normal paths and edge cases.
- CI SHALL build and run tests on each push/PR.

### NFR-4 Safety
- No undefined behavior from missing returns, out-of-bounds indexing, or uninitialized conversion state.
- Development builds SHOULD support sanitizers (ASAN/UBSAN) for debugging.

## 6. Acceptance Criteria (v0.1)
v0.1 is considered done when:
1. All modules compile cleanly.
2. Unit tests for ticks, ring buffer, and histogram pass in CI.
3. The benchmark harness runs successfully and prints:
   - N, WARMUP
   - histogram unit (ns), bucket size, max cap
   - count, p50_ns, p99_ns, p999_ns

## 7. Traceability
- Tick API → Module `perf/ticks`
- Ring buffer → Module `core/spsc_ring`
- Histogram → Module `perf/latency_hist`
- Harness → `app/bench`
