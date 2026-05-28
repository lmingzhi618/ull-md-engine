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

Important note: current `try_push()` may spin internally until the slot becomes available. Therefore measured latency includes producer-side contention and slot wait time.

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

## Capacity Sensitivity

To test whether high end-to-end latency is caused by queue backlog, the benchmark was repeated with different ring capacities.
Command shape:
```bash
./scripts/run_mpsc_ring_rel <producers> 500000 50000 <capacity>
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

### Interpretation
The results confirm that sustained end-to-end latency is dominated by queue backlog.
For both 4 and 8 producers, throughput stays in roughly the same range as capacity changes, while p50 latency scales almost linearly with ring capacity.
This matches the expected queueing relationship:
> latency ~= queue_depth / consumer_drain_rate

When the producer side continuously supplies more work than the single consumer can drain immediately, the ring remains close to full. A larger capacity therefore allows more messages to accumulate before the consumer reaches them.
For example, with 4 producers and capacity 65,536:
> 65,546 / 4.597M msg/s ~= 14.25ms 

The measured p50 latency was:
> 14.799 ms 

With capacity 1024:
> 1,024 / 4.505M msg/s ~= 0.227 ms 

The measured p50 latency was:
> 0.230 ms 

This explains why smaller capacity produced lower latency in this benchmark. It does not make the consumer faster; it limits how much backlog can accumulate.
The tradeoff is that smaller capacity reduces burst absorption. Larger capacity can absorb short bursts, but under sustained overload it mostly converts excess supply into queueing latency.

## Observations

Throughput decreases as producer count increases. The queue does not scale linearly with producer count in this initial design.

Latency remains low for 1-2 producers, but jumps sharply at 4 producers. At 4 producers, p50 is already above 13ms, which means the high latency is not just a rare tail event.

At 8 producers, p99 and p999 hit the histogram cap of 20ms. The true tail latency is therefore greater than or equal to 20ms.

## Interpretation

The likely source of contention is the shared producer-side head_.fetch_add() combined with bounded-ring slot waiting. Multiple producers contend for the same atomic counter and may also spin waiting for slots to become reusable.

Because timestamps are taken before try_push(), the measured latency includes:

producer ticket contention
producer waiting for ring slot availability
queue residence time
consumer receive time

## Next Questions

How much time is spent inside producer try_push()?
How much latency remains after the message is published to the ring?
Would padding head_, tail_, and cells reduce contention?
Would an adaptive spin strategy reduce tail latency?
How does a true bounded non-blocking try_push() change behavior?
