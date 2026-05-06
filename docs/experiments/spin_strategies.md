# Spin Strategies - Latency Impact in SPSC Queue

## Environment
- Machine: Apple MacBook Pro (M2 Pro)
- OS: macOS
- Compiler: Clang (C++20)
- Build: Release (-O3)
- Queue: Lock-free SPSC ring buffer
- Threads: 1 producer, 1 consumer

## Benchmark Design
I evaluate different spin-wait strategies in a producer-consumer pipeline.
Configuration:
- N = 2,000,000 messages
- Warmup = 200,000
- Latency measured as:
  - consumer_receive_time - producer_send_time
- Histogram:
  - unit: nanoseconds
  - bucket: 50 ns 
  - max range: 20 ms 

## Spin Strategies

### 1. Pure Spin
Busy loop with no pause or yield

### 2. CPU Relax 
Uses architecture-specific pause instruction (x86 `pause`, ARM `yield`).

### 3. Thread Yield 
Calls `std::this_thread::yield()` when waiting. 

### 4. Exponential Backoff 
Gradually increases waiting intensity based on retry attempts.

### 5. Hybrid (Spin-Then-Yield)
Spins for a short period, then yields if contention persists.

## Results (Summary)

### Pure Spin 
- p50: ~2.41-2.55 ms
- p99: ~3.69-4.21 ms 
- p999: ~3.73-4.35 ms 

### CPU Relax 
- p50: ~2.40-2.49 ms 
- p99: ~3.76-4.14 ms 
- p999: ~3.84-4.16 measured

### Thread Yield 
- p50: ~2.40-2.45 ms 
- p99: ~2.57-3.70 ms 
- p999: ~2.65-3.71 ms 

### Backoff
- p50: ~2.43-2.64 ms 
- p99: ~3.83-4.14 ms 
-p999: ~3.86-4.15 ms 

### Adaptive 
- p50: ~2.39-2.48 ms 
- p99: ~2.54-4.07ms 
- p999: ~2.55-4.15 ms 

## Key Findings 
1. Median latency (p50) is similar across all strategies. 
2. Tail latency (p99/p999) differs significantly:
    - Thread yield consistently achieves the lowest tail latency.
    - Pure spin exhibits higher tail latency due to contention. 

3. CPU relax and backoff behave similarly in this workload. 
4. Backoff does not show clear benefits because contention is not sustained long enough.
5. Adaptive spin can occasionally find a better operating point, but the current simple threshold update rule is not stable enough to consistently outperform static strategies on macOS/M2 Pro.

Static and adaptive spin strategies primarily affect tail latency rather than median latency. On macOS/M2 Pro, yield-based strategies often improve tail behavior, while the initial adaptive strategy shows promising best-case results but requires better feedback control for consistent gains.

## Interpretation

### Why Pure Spin Performs Worse in Tail 
Pure spinning keeps threads constantly active, which increases CPU contention and can delay scheduling of the counterpart thread.

### Why Yield Improves Tail Latency 
Yielding allows the scheduler to switch execution between producer and consumer more fairly, reducing contention and improving tail latency.

### Platform-Specific Behavior 
This result is influenced by maxOS scheduling behavior and heterogeneous core (M2 Pro), On Linux systems with strong CPU affinity, pure spinning with `pause` is often preferred.

### Why Backoff Shows Limited Impact
The queue rarely experiences prolonged contention, so the backoff strategy does not reach its higher-intensity waiting phases.

## Takeaway 

- Spin strategy has little impact on median latency but significantly affects tail latency.
- Pure spinning maximizes responsiveness but increases contention.
- Yield-based strategies improve fairness and reduce tail latency on maxOS.
- Hybrid spin-then-yield is expected to provide the best balance in production systems.


