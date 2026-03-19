# Busy Polling vs Blocking Queue — Latency Benchmark

## 1. Machine / Environment

- **Machine**: Apple MacBook Pro (M2 Pro)
- **CPU Architecture**: ARM64
- **OS**: macOS
- **Compiler**: Clang (C++20)
- **Build Type**: Release (`-O3`)
- **Sanitizers**: Disabled
- **Threads**:
  - 1 Producer thread
  - 1 Consumer thread
- **CPU Affinity**: Not pinned (default OS scheduling)


## 2. Benchmark Design

This experiment compares two producer–consumer communication models:

### (1) Busy Polling (SPSC Ring Buffer)

- Lock-free `SpscRing<T>`
- Producer:
  - `try_push()` in a spin loop
- Consumer:
  - `try_pop()` in a spin loop
- No blocking, no syscalls


### (2) Blocking Queue

- `BlockingQueue<T>` implemented with:
  - `std::mutex`
  - `std::condition_variable`
- Producer:
  - `push()` (may block when full)
- Consumer:
  - `pop()` (blocks when empty)
- Uses OS scheduler for wakeup


### Common Configuration

- Message count: `N = 2,000,000`
- Warmup: `200,000` messages
- Histogram:
  - Unit: nanoseconds
  - Bucket size: 50 ns
  - Max range: 20 ms
- Latency definition:

```text
latency = consumer_receive_time - producer_send_time
```

* Timing:
    * ticks() + ticks_to_ns() abstraction
    * Monotonic and cross-platform


## 3. Busy Polling Results

Typical results (multiple runs):

```code
mode=busy
p50 ≈ 2.44–2.50 ms
p99 ≈ 3.70–4.10 ms
p999 ≈ 3.76–4.12 ms
```

Observations:

 *	Low median latency
 *	Tight tail distribution
 *	Stable across runs

## 4. Blocking Queue Results

Typical results:
```code
mode=blocking
p50 ≈ 4.1–8.3 ms
p99 ≈ 8.5–8.7 ms
p999 ≈ 8.5–8.7 ms
```

Observations:

* Significantly higher median latency
* Much worse tail latency
* Noticeable run-to-run variance (scheduler effects)

---

## 5. Interpretation

### 5.1 Scheduler Overhead

Blocking queues rely on:
	•	condition_variable::wait()
	•	OS-level thread sleep/wakeup
	•	Mutex reacquisition

This introduces:
	•	Context switch latency
	•	Scheduling delays
	•	Wake-up uncertainty


### 5.2 Busy Polling Advantages

Busy polling:
	•	Avoids kernel transitions
	•	Avoids context switches
	•	Keeps threads actively running on CPU

This results in:
	•	Lower median latency
	•	Better tail latency (p99 / p999)
	•	More deterministic behavior


### 5.3 Tail Latency Amplification

In the blocking model:

* Even small wake-up delays (~tens of microseconds)

* Accumulate into millisecond-level latency

This explains:
	* p99/p999 nearly saturating at ~8–9 ms


### 5.4 Variability

Blocking mode shows higher variance due to:
	•	OS scheduling policies
	•	Background system activity
	•	Thread wake-up timing

Busy mode is more stable because it is:
	•	CPU-bound
	•	Less dependent on OS decisions

## 6. Takeaway

### Key Result

Busy polling significantly outperforms blocking queues in both median and tail latency on this system.


### Engineering Insight

* Blocking primitives improve CPU efficiency
* But introduce unpredictable latency due to scheduler involvement


### Low-Latency System Implication

This experiment demonstrates a fundamental trade-off:

| **Model**    | **CPU Usage** | **Latency** | **Determinism** |
| ------------ | ------------- | ----------- | --------------- |
| Busy Polling | High          | Low         | High            |
| Blocking     | Low           | High        | Low             |

## **Conclusion**

### In latency-sensitive systems (e.g., trading, market data processing), busy polling is often preferred despite higher CPU cost, because it eliminates scheduler-induced latency and provides tighter tail behavior.
