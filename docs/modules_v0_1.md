# Module Design v0.1 — ULL Market Data Engine

This document describes module boundaries, responsibilities, public interfaces, and unit-test requirements for v0.1.

## 1. Module Overview
v0.1 consists of three core modules plus one integration harness:

1. `perf/ticks` — monotonic tick source + conversion to nanoseconds
2. `core/spsc_ring` — single-producer/single-consumer ring buffer
3. `perf/latency_hist` — latency histogram + percentile summary
4. `app/bench` — integration harness (not a unit-tested module; tested via smoke run)

All module APIs are defined in headers under `include/ull/...` with implementations in `src/...`.

---

## 2. Module: perf/ticks

### 2.1 Responsibility
Provide a monotonic tick counter for measuring time intervals and a conversion function to nanoseconds.  
This module enforces **unit consistency** for the entire project.

### 2.2 Public Interface
Header: `include/ull/perf/ticks.h`

- `void init_ticks();`
  - One-time initialization/calibration.
  - On x86, may calibrate TSC frequency.
  - On non-x86, may be a no-op.

- `std::uint64_t ticks() noexcept;`
  - Returns a monotonic tick.
  - No unit guarantee.

- `std::uint64_t ticks_to_ns(std::uint64_t delta) noexcept;`
  - Converts a tick delta into nanoseconds.
  - Must be safe for delta=0.

### 2.3 Contract
- `ticks()` SHALL be monotonic within the same thread.
- `ticks_to_ns(0) == 0`.
- If calibration is required, `init_ticks()` MUST be called before `ticks_to_ns()` is used for reporting.

### 2.4 Unit Tests
File: `tests/test_ticks.cpp`

Minimum coverage:
1. `ticks()` is non-decreasing over many calls.
2. `ticks_to_ns(0) == 0`.
3. (Smoke) measure a short sleep; converted delta should be > 0.

---

## 3. Module: core/spsc_ring

### 3.1 Responsibility
Provide a high-performance fixed-capacity FIFO queue for exactly one producer and one consumer.

### 3.2 Public Interface
Header: `include/ull/core/spsc_ring.h`

- `explicit SpscRing(std::size_t capacity_pow2);`
- `bool try_push(const T& v) noexcept;`
- `bool try_pop(T& out) noexcept;`
- `std::size_t capacity() const noexcept;`

### 3.3 Contract
- FIFO ordering for all successfully enqueued items.
- `try_push` returns `false` if full (and SHALL NOT block).
- `try_pop` returns `false` if empty (and SHALL NOT block).
- No allocations after construction.
- SPSC only: behavior is undefined if used with multiple producers or multiple consumers.

### 3.4 Unit Tests
File: `tests/test_spsc_ring.cpp`

Minimum coverage:
1. Empty pop returns false.
2. Push then pop returns true and preserves payload.
3. Fill to capacity → next push returns false.
4. Wrap-around: push/pop enough times to cross the ring boundary while preserving FIFO.

---

## 4. Module: perf/latency_hist

### 4.1 Responsibility
Record latency samples (in nanoseconds) into a histogram and compute percentiles for reporting.

### 4.2 Public Interface
Header: `include/ull/perf/latency_hist.h`

- `LatencyHist(std::uint64_t max_ns, std::uint64_t bucket_ns);`
- `void add(std::uint64_t ns) noexcept;`
- `std::string report() const;`

### 4.3 Contract
- Input unit is nanoseconds.
- `add(ns)` SHALL cap `ns > max_ns` to `max_ns`.
- `report()` SHALL return a string with:
  - `count=...`
  - `p50_ns=...`
  - `p99_ns=...`
  - `p999_ns=...`
- Percentiles should be monotonic: `p50 <= p99 <= p999`.

### 4.4 Unit Tests
File: `tests/test_latency_hist.cpp`

Minimum coverage:
1. Cap behavior: adding `ns > max_ns` does not crash and increments last bucket.
2. Deterministic percentile: construct a known distribution and validate p50/p99.
3. `report()` returns a non-empty string and includes required keys.

---

## 5. Integration Harness: app/bench

### 5.1 Responsibility
Drive producer/consumer threads using `SpscRing` and measure end-to-end enqueue→dequeue latency.

### 5.2 Inputs/Outputs
- Inputs: `N` (message count), `WARMUP` (discard first observations).
- Output: stable-format report including config lines and histogram summary.

### 5.3 Notes
- The bench is not treated as a unit-tested module.
- It should be run as a smoke test locally; CI may optionally run a very small bench later.

---

## 6. Build and CI Notes (v0.1)
- CI compiles the library and runs unit tests via `ctest`.
- Development builds may enable sanitizers (ASAN/UBSAN) for debugging.
- Release builds are used for benchmark runs.