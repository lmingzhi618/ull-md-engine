# Project Status - ull-md-engine

## Project Goal

Build a staged ultra-low-latency market data engine and experimentation platform focused on:

- lock-free concurrency
- latency engineering 
- cache and scheduler behavior
- advanced queue design 
- memory optimization 
- NUMA and allocator control
- reproducible benchmarking 
- systems performance research 

---

## Tagged Roadmap 

- v0.1 -> MVP pipeline
- v0.2 -> Engineering version (B)
- v0.3 -> Advanced concurrency
- v0.4 -> Memory optimization 
- v0.5 -> NUMA + allocator control 

---

# v0.1 - MVP Pipeline 

## Goal 

Runnable, measurable low-latency market data engine 

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

Status: Completed

---

# v0.2 — Engineering Experiments

## Goal

Increase technical signal strength through latency engineering experiments.

## Completed

### 1. Cache Padding / False Sharing

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

Status: Completed enough for v0.2 tag

Recommended README wording:
- profiling / hotspot analysis
- call graph analysis
- avoid claiming full Linux perf flamegraph unless later done on Linux

---

# v0.3 — Advanced Concurrency

## Goal

Move from low-contention SPSC to real contention and advanced concurrent queue design.

## Current Focus

Bounded lock-free MPSC ring buffer.

---

## Task 1: MPSC Ring Correctness

### Goal

Implement a bounded MPSC queue using per-slot sequence protocol.

### Concepts

- many producers
- single consumer
- producer contention
- `fetch_add` ticket allocation
- per-slot sequence numbers
- claim vs publish separation
- acquire/release ordering

### Implementation Plan

Files:
- `include/ull/core/mpsc_ring.h`
- `tests/test_mpsc_ring.cpp`
- `scripts/run_mpsc_ring_rel.sh`

### Design

Each cell:

```cpp
struct Cell {
  std::atomic<std::uint64_t> seq;
  T value;
};

Producer flow:

claim position with head_.fetch_add(1)
map logical position to physical slot with pos & mask_
wait until cell.seq == pos
write payload
publish with cell.seq.store(pos + 1, release)

Consumer flow:

inspect tail_
check cell.seq == tail_ + 1
read payload
recycle with cell.seq.store(tail_ + capacity, release)
increment tail_
Important API Note

Initial version may block/spin inside push path.

If try_push() can wait indefinitely, rename it to:

void push(const T& v)

A true bounded try_push() should return false when full.

Tests
constructor / capacity
empty pop
single producer push/pop
multiple producers
total count correctness
per-producer count correctness
no lost messages
no duplicated messages

Status: In progress

Task 2: True Bounded try_push
Goal

Implement non-blocking push semantics.

Requirements
detect full queue before claiming too far ahead
return false instead of spinning forever
avoid breaking sequence protocol
preserve correctness under multiple producers
Expected Challenges
head reservation may advance too far
full detection is harder than SPSC
may need CAS instead of unconditional fetch_add

Status: Planned

Task 3: MPSC Benchmark
Goal

Measure contention cost as producer count increases.

Benchmark Modes
1 producer
2 producers
4 producers
8 producers

Metrics:

throughput
p50 / p99 / p999 latency
producer contention effect
tail amplification
failed retries if CAS-based version is implemented

Files:

src/app/mpsc_bench_main.cpp
scripts/run_mpsc_bench_rel.sh
docs/experiments/mpsc_contention.md

Status: Planned

Task 4: Memory Ordering Documentation
Goal

Document acquire/release reasoning for MPSC.

Topics:

why producer publish uses release
why consumer readiness check uses acquire
why claim and publish are separate
why tail can be non-atomic
why per-slot sequence avoids reading partially written payload

Docs:

docs/design/mpsc_memory_ordering.md

Status: Planned

Task 5: Disruptor-style Sequencer Exploration
Goal

Evolve ring buffer thinking toward Disruptor-like architecture.

Topics:

sequence abstraction
publish barrier
gating sequence
fanout consumers
consumer dependency graph
wait strategies

Possible files:

docs/design/disruptor_notes.md

Status: Planned

v0.4 — Memory Optimization
Goal

Reduce allocation, layout, and cache-related jitter.

Planned Tasks
1. Custom Memory Pool / Slab Allocator
fixed-size object pool
preallocation
no runtime heap allocation on hot path
benchmark allocator jitter
2. Structure Layout Tuning
field ordering
cache-line alignment
padding analysis
hot/cold field separation
3. Branch / Prefetch Experiments
branch prediction effects
likely/unlikely annotations if useful
manual prefetch experiments
measure impact on tail latency
4. Replay + Snapshot
replay binary market data stream
snapshot state
replay deterministic benchmark workload

Status: Planned

v0.5 — NUMA + Allocator Control
Goal

Move toward production-like low-latency infrastructure concerns.

Planned Tasks
1. NUMA-aware Pipeline
producer/consumer placement
memory locality
per-node allocation
Linux-only experiments
2. Allocator Jitter Control
compare system allocator vs custom pool
observe tail latency impact
document allocation-related jitter
3. Burst Traffic Modeling
simulate bursty market data
measure queue backlog
measure tail amplification
evaluate backpressure policies
4. Linux perf / FlameGraph
run on Linux environment
perf record
perf script
stackcollapse-perf.pl
flamegraph.pl
compare with macOS call graph profiling

Status: Planned

Current Immediate Next Steps
Finish MPSC correctness implementation
Make API semantics clean:
push() if blocking/spinning
try_push() only if truly non-blocking
Add run_mpsc_ring_rel.sh
Ensure test_mpsc_ring passes reliably
Commit MPSC correctness milestone
Start MPSC benchmark
Current Technical Notes
Ring Capacity

Ring capacity must be power-of-two because:

idx = pos & (capacity - 1);

This is equivalent to:

idx = pos % capacity;

only when capacity is a power of two.

Benefits:

avoids expensive modulo
enables efficient logical sequence to physical slot mapping
common in Disruptor-style and lock-free ring buffers
Percentiles
p50: median latency
p99: tail latency, slowest 1%
p999: extreme tail latency, slowest 0.1%

p99/p999 are critical because low-latency systems care more about worst-case behavior than average behavior.

Project Positioning

This project is no longer just a queue implementation.

It is a staged low-latency systems engineering portfolio covering:

lock-free queues
cache effects
tail latency
scheduler behavior
spin strategies
profiling
advanced concurrency
future NUMA / allocator control
