# MPSC Contention Experiment

## Goal

Measure how the initial bounded MPSC ring behaves as producer count increases.

## Queue Under Test

- Queue: `MpscRing<T>`
- Design: per-slot sequence protocol
- Producer claim: global `head_.fetch_add()`
- Producer behavior: spinning push path
- Consumer: single busy-polling consumer
- Capacity: 65536

Important note: the initial `push()` path spins until a slot becomes available. Latency measured in push mode includes producer-side contention, slot wait time, queue residence time, and consumer receive time.

## Benchmark Setup

Command:

```bash
./scripts/run_mpsc_ring_rel.sh <producers> 500000 50000
```

## Parameters:

- producers: 1, 2, 4, 8
- messages per producer: 500,000
- warmup messages: 50,000
- latency histogram max: 20ms
- latency histogram bucket: 50ns

## Results

|Producers | Total Messages	| Throughput msg/s | p50 ns	    | p99 ns	 | p999 ns	  | Notes |
|---:|---:|---:|---:|---:|---:|---:|
|1	       | 500,000	    | 8.33M	           | 150	    | 288,400	 | 333,300	  | baseline |
|2	       | 1,000,000	    | 7.80M	           | 600	    | 18,750	 | 37,950	  | mild contention |
|4	       | 2,000,000	    | 5.24M	           | 13,098,300	| 14,995,250 | 15,196,750 | severe contention |
|8	       | 4,000,000	    | 3.97M	           | 16,377,650	| 20,000,000 | 20,000,000 | p99/p999 capped |

## Initial Observations

Throughput decreases as producer count increases. The queue does not scale linearly with producer count in this initial design.

Latency remains low for 1-2 producers, but jumps sharply at 4 producers. At 4 producers, p50 is already above 13ms, which means the high latency is not just a rare tail event.

At 8 producers, p99 and p999 hit the histogram cap of 20ms. The true tail latency is therefore greater than or equal to 20ms.

## Initial Interpretation

The initial result suggested producer contention and bounded-ring waiting, but later capacity and push-latency instrumentation showed that sustained queue backlog is the dominant source of end-to-end latency at 4+ producers. Multiple producers contend for the same atomic counter and may also spin waiting for slots to become reusable.

Because timestamps are taken before `push()` in push mode, the measured latency includes:

producer ticket contention
producer waiting for ring slot availability
queue residence time
consumer receive time


## Capacity Sensitivity

To test whether high end-to-end latency is caused by queue backlog, the benchmark was repeated with different ring capacities.
Command shape:

```bash
./scripts/run_mpsc_ring_rel.sh push <producers> 500000 50000 <capacity>
```

### 4 Producers  
| Capacity	| Throughput msg/s  | p50 ns	 | p99 ns	    | p999 ns |
|---:|---:|---:|---:|---:|
| 1,024	    | 4.50M	            | 230,550	 | 256,750	    | 270,650 |
| 4,096	    | 4.52M	            | 921,650	 | 1,006,700	| 1,020,000 |
| 16,384	| 4.55M	            | 3,669,200	 | 4,011,750	| 4,099,000 |
| 65,536	| 4.60M	            | 14,799,250 | 15,561,850	| 15,602,750 |

### 8 Producers

| Capacity	| Throughput msg/s | p50 ns	    | p99 ns	    | p999 ns |
|---:|---:|---:|---:|---:|
| 1,024	    | 3.62M	           | 280,200	| 335,450	    | 1,487,000 |
| 4,096	    | 3.69M	           | 1,105,750	| 1,279,800	    | 2,389,850 |
| 16,384	| 3.68M	           | 4,479,400	| 4,795,550	    | 5,999,750 |
| 65,536	| 3.59M	           | 18,006,800	| 20,000,000	| 20,000,000 |


### Capacity Interpretation
The results confirm that sustained end-to-end latency is dominated by queue backlog.
For both 4 and 8 producers, throughput stays in roughly the same range as capacity changes, while p50 latency scales almost linearly with ring capacity.
This matches the expected queueing relationship:
> latency ~= queue_depth / consumer_drain_rate

When the producer side continuously supplies more work than the single consumer can drain immediately, the ring remains close to full. A larger capacity therefore allows more messages to accumulate before the consumer reaches them.
For example, with 4 producers and capacity 65,536:
> 65,536 / 4.597M msg/s ~= 14.25ms 

The measured p50 latency was:
> 14.799 ms 

With capacity 1024:
> 1,024 / 4.505M msg/s ~= 0.227 ms 

The measured p50 latency was:
> 0.230 ms 

This explains why smaller capacity produced lower latency in this benchmark. It does not make the consumer faster; it limits how much backlog can accumulate.
The tradeoff is that smaller capacity reduces burst absorption. Larger capacity can absorb short bursts, but under sustained overload it mostly converts excess supply into queueing latency.


## Drop-on-Full Mode 

The benchmark was extended with a `try_drop` mode using the non-blocking MPSC `try_push` API.
In this mode, producers attempt to publish each message once. If the queue is full, the message is dropped instead of spinning until a slot becomes available.

Command shape:

```bash
./scripts/run_mpsc_ring_rel.sh try_drop <producers> 500000 50000 <capacity>
```

### 4 Producers 

|Capacity	|Published	|Dropped	|Consumed	|Throughput msg/s	    |p50 ns	    |p99 ns	    |p999 ns|
|----------:|----------:|----------:|----------:|----------------------:|----------:|----------:|------:|
|256	    |479,446	|1,520,554	|479,446	|8.80M	                |125,250	|144,000	|165,000|
|512	    |526,086	|1,473,914	|526,086	|8.38M	                |237,150	|269,100	|278,250|
|1,024	    |498,937	|1,501,063	|498,937	|8.62M	                |509,250	|544,350	|607,400|
|4,096	    |539,152	|1,460,848	|539,152	|8.27M	                |1,950,750	|2,151,300	|2,186,800|

### Drop-on-Full Observations

try_drop keeps producer-side push latency low because producers do not wait for queue capacity. Full queues return false immediately and the benchmark counts the messages as dropped.

End-to-end latency still scales with queue capacity. Smaller capacity creates a tighter bound on backlog, while larger capacity allows more stale work to accumulate before the consumer reaches it.

The drop count remains high across all tested capacities because producer supply exceeds the single consumer's drain rate. Capacity changes how much backlog is allowed, but it does not remove the consumer bottleneck.

### Drop-on-Full Interpretation 

try_drop demonstrates an overload tradeoff:
```text
low queueing latency 
higher message loss
```
This mode is useful as a systems experiment, but it is not a complete market-data correctness policy.
For sequence-dependent market data streams, dropping arbitrary messages can make downstream state invalid. For example, if an incremental book update is dropped, the consumer may observe a sequence gap and the local book can no longer be trusted.
A production market-data engine would need a domain-aware overload policy, such as:
- detecting sequence gaps and triggering resync
- dropping and rebuilding from snapshot 
- coalescing latest-only values when correctness allows it 
- sharding by instrument or channel
- shedding lower-priority symbols or feeds 

Queue capacity behaves like a user-space backlog: it can absorb bursts, but under sustained overload it directly controls how much stale work may accumulate before the consumer catches up.



## Next Questions
- How should the engine choose capacity from a target latency budget?
- How should market-data-specific overload policy work beyond arbitrary drop-on-full?
- Would padding head_, tail_, and cells reduce contention?
- Would an adaptive spin strategy reduce tail latency in push mode?
- How should dropped messages interact with sequence-gap detection and resync?
