# Single Producer Event Pipeline Baseline 

## Question 

What is the baseline throughput and end-to-end latency of the current `SingleProducerEventPipeline`?

This experiment measures the first complete Disruptor-style pipeline:

```text
SingleProducerSequencer
-> SequencedRing 
-> SequenceBarrier 
-> WaitStrategy 
```

The goal is not to measure the cost of a single primitive operation. The goal is to measure queue residency time in a complete producer/consumer pipeline.

## Setup 

Command shape:
```bash
./build/rel/sp_pipeline_bench <messages> <capacity>
```
Run parameters:
```text
messages = 1,000,000
warmup   = 50,000
layout   = single producer / single consumer
wait     = default busy spin
hist     = ns, bucket_ns=50, max_ns=20,000,000
```
The producer records a timestamp before publishing each event. The consumer waits for visibility, reads the timestamp, and records:
```text 
consumer_now - producer_timestamp
```
So the measured latency includes time spent waiting inside the ring.

## Results 

| Capacity | Throughput msg/s | p50 ns | p99 ns | p999 ns |
|---:|---:|---:|---:|---:|
| 256 | 12.4206M | 14,550 | 46,900 | 50,500 |
| 1024 | 12.5723M | 65,750 | 165,550 | 181,850 |
| 4096 | 13.0914M | 256,000 | 685,050 | 728,350 |
| 16384 | 10.8379M | 1,354,850 | 2,550,650 | 2,588,000 |

## Observation

Increasing capacity significantly increases measured latency. Theoughput stays in the same broad range of capacities 256 to 4096, then drops at capacity 16384.
This suggests that capacity is not improving steady-state consumer speed. Instead, larger capacity allows the producer to run further ahead of the consumer.

## Interpretation 

The measured latency is queue residence time:
```text 
producer timestamp 
-> publish 
-> wait in pipeline backlog 
-> consumer read 
-> latency sample 
```

A larger ring can absorb more burst, but it can also hold older events for longer. This increases p50/p99/p999 latency even when throughput does not improve.

This matches the earlier MPSC capacity experiments:
```text 
larget capacity 
-> deeper backlog
-> higher end-to-end latency 
````

Capacity should therefore be treated as a latency/backpressure trade-off, not as a pure performance improvement.

## Limitations 

This benchmark currently uses:
- one producer
- one consumer 
- default busy-spin wait strategy 
- timestamp payload only 
- no thread affinity 
- no comparision against SPSC or MPSC in the same benchmark 

The result is a useful baseline, but not yet a final performance characterization.

## Next Step 

Compare the single-producer event pipeline against the existing SPSC ring under a similar one-producer / one-consumer benchmark shape.
That will answer whether the sequencer/barrier abstraction adds measureable overhead compared with the simpler SPSC queue.

