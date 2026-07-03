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

Increasing capacity significantly increases measured latency. Throughput stays in the same broad range of capacities 256 to 4096, then drops at capacity 16384.
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
larger capacity 
-> deeper backlog
-> higher end-to-end latency 
```

Capacity should therefore be treated as a latency/backpressure trade-off, not as a pure performance improvement.

## Limitations 

This benchmark currently uses:
- one producer
- one consumer 
- default busy-spin wait strategy 
- timestamp payload only 
- no thread affinity 
- no comparison against SPSC or MPSC in the same benchmark 

The result is a useful baseline, but not yet a final performance characterization.


## SPSC comparison 

A second benchmark compares `SingleProducerEventPipeline` against the existing `SpscRing` using the same benchmark shape:
```text 
producer timestamp 
-> queue/pipeline  
-> consumer read timestamp 
-> latency sample 
```
Each configuration was run 5 times.
The table below reports median values.
| Capacity | SPSC throughput | Pipeline throughput | SPSC p50 ns | Pipeline p50 ns |
|---:|---:|---:|---:|---:|
| 256 | 12.61M | 13.26M | 13,850 | 16,400 |
| 1024 | 17.18M | 17.09M | 43,600 | 52,800 |
| 4096 | 18.63M | 16.36M | 178,050 | 209,200 |
| 16384 | 17.70M | 17.80M | 719,100 | 806,450 |

The pipeline usually has slightly higher latency than the specialized SPSC ring. This is expected because the pipeline makes the lifecycle explicit:
```text 
claim -> write -> publish -> wait -> read -> consume 
```

The SPSC ring is a more specialized queue and has a shorter hot path.
However, the dominant effect is still capacity/backlog. Increasing capacity raises queue residency time for both implementations much more than the sequencer/barrier abstraction itself.

Throughput is noisier than latency in this benchmark. Without thread affinity, small throughput differences should not be over-interpreted.

## Fanout Baseline 

The next benchmark measures a shared-ring fanout topology:
```text 
SingleProducerSequencer 
-> SequencedRing 
-> SequenceBarrier per consumer 
-> GatingSequences
```
Command shape:
```bash 
./build/rel/sp_fanout_bench <consumers> <messages> <capacity>
```
Run parameters:
```text 
messages = 1,000,000
capacity = 1024
warmup   = 50,000
wait     = default busy spin
hist     = ns, bucket_ns=50, max_ns=20,000,000
```

Each event is written once by the producer and read by every consumer. Producer capacity is controlled by the slowest consumer:
```text 
available capacity depends on min(all consumer sequences)
```

### Results 

The benchmark was run multiple times. Throughput was noisy without thread affinity, so the table below reports the stable trend rather than treating a single run as authoritative.

| Consumers | Throughput range |
|---:|---:|
| 1 | 14M - 23M msg/s |
| 2 | 10M - 12.6M msg/s |
| 4 | 5.1M - 5.4M msg/s |

### Observation

Throughput decreases as the number of consumers increases.
This is expected because each message is now consumed by multiple threads, and the producer cannot reuse a slot until every consumer has advanced past the old sequence.
The current implementation also makes producer capacity checks more expensive as consumers increase:
```text 
sequence.next() 
-> remaining_capacity() 
-> gating_min() 
-> scan all consumer sequences 
```

So the producer hot path includes an O(number_of_consumers) gating scan.
Consumer latency distributions also diverge. Some consumers show very low p50 latency, while others show much higher p50 or tail latency. This reflects phase differences between consumers:
```text 
fast consumer:
    waits near the next sequence 
    reads soon after publish 
    low observed latency 

slow consumer:
    trails behind producer
    reads older timestamp 
    higher observed latency 
```

The overall producer is still limited by the slowest consumer because slot reuse is gated by the minimum consumer sequence.

### Interpretation 

This benchmark demonstrates the main Disruptor-style fanout trade-off.

Benefits:

```text 
producer writes each event once 
multiple consumers read the same ordered stream 
sequence ordering is shared across consumers 
backpressure is explicit 
```

Costs:

```text 
producer is constrained by the slowest consumer 
consumer count increases gating overhead 
busy-spin consumers increase scheduler pressure 
tail latency becomes sensitive to slow or descheduled consumers 
```

This makes the fanout model useful for pipelines where shared ordering and shared storage matter, but it is not a free replacement for a simple point-to-point SPSC queue.

### Limitations 

This benchmark currently has no thread affinity. With multiple busy-spinning consumers, OS scheduling noise can strongly affect both throughput and latency.

The next step should therefore be to add optional thread affinity to the fanout benchmark before drawing stronger conclusions from 4-consumer results.

## Next Step 

Add optional thread affinity to `sp_fanout_bench`.
This should reduce scheduler noise and make the fanout measurements more interpretable, especially for 4-consumer results.
