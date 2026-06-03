# MPSC Overload Policy Notes

## Problem 

The MPSC benchmark shows that sustained overload creates two bad choices:

- `push` mode preserves all messages but turns backlog into high latency.
- `try_drop` mode bounds backlog but creates sequence gaps.

For sequence-dependent market data, arbitrary drop-on-full is not a complete correctness policy.

## Policy Options


### 1. Lossless push

All messages are preserved. Producers wait when the queue is full.

Best for:
- sequence-dependent incremental feeds
- order book construction 
- correctness-first replay

Tradeoff:
- latency can grow with queue capacity under sustained overload 

### 2. drop-on-Full with Gap Detection

Messages may be dropped when the queue is full. The consumer detects gaps using sequence numbers.

Best for:
- experiments
- feeds where resync is available

Required follow-up:
- mark affected stream as stale
- request snapshot / resync 
- ignore dependent updates until recovery 

### 3. Latest-Value Coalescing 

Only the latest value per key is perserved.

Best for:
- top-of-book display
- UI/monitoring
- non-critical derived signals

Not safe for:
- full depth order book incremental updates 

## Current Conclusion
Queue overload policy must be domain-aware. Lower latency cannot be achieved only by making the queue drop messages; the engine must also define what dropped messages mean for downstream state.

