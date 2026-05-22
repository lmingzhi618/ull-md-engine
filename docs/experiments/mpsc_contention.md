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

## Parameters:

- producers: 1, 2, 4, 8
- messages per producer: 500,000
- warmup messages: 50,000
- latency histogram max: 20ms
- latency histogram bucket: 50ns

## Results

|Producers | Total Messages	| Throughput msg/s | p50 ns	    | p99 ns	 | p999 ns	  | Notes
|1	       | 500,000	    | 8.33M	           | 150	    | 288,400	 | 333,300	  | baseline
|2	       | 1,000,000	    | 7.80M	           | 600	    | 18,750	 | 37,950	  | mild contention
|4	       | 2,000,000	    | 5.24M	           | 13,098,300	| 14,995,250 | 15,196,750 | severe contention
|8	       | 4,000,000	    | 3.97M	           | 16,377,650	| 20,000,000 | 20,000,000 | p99/p999 capped

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
