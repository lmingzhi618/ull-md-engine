# v0.3 Concurrency Architecture

## Goal

v0.3 focuses on advanced concurrency patterns for a low-latency market data engine.

The goal is not to build a full Disruptor immediately. The goal is to understand the core mechanisms behind high-performance queues and event pipelines:

- how producers claim space safely
- how publication is separated from claiming 
- how consumers observe published data 
- how capacity prevents overwriting unread data
- how sequencers can separate ordering from storage

This phase treats the current MPSC ring as the concrete baseline, then uses small sequencer experiments to move toward a Disruptor-style model.

## Current Components

Current v0.3 components:

- `MpscRing`
  - bounded multi-producer, single-consumer ring 
  - producers claim positions using a shared atomic head 
  - each cell has its own sequence state
  - consumer advances a single tail position 

- `Sequence` 
  - cache-line-aligned atomic sequence wrapper 
  - used to make producer/consumer progress explicit
  - provides load/store/fetch/CAS operations 

- `SingleProducerSequencer`
  - separates sequence claiming from publication 
  - tracks producer cursor and consumer gating sequence 
  - controls capacity without owning event storage 

- MPSC benchmark 
  - compares `push` and `try_drop` 
  - compares different capacities 
  - compares padded and cell-padded layouts 
  - records throughput, latency, drops, and sequence gaps 

## MPSC Ring Model

The current MPSC ring combines three responsibilities:

- producer claim
- data storage
- publication visibility 

Producer flow:
```text 
claim position -> wait for reusable cell -> write payload -> publish cell 
```

Consumer flow:
```text 
check next tail -> read published cell -> recycle cell -> advance tail 
``` 

This model is concrete and practical. It is a good baseline because it shows real contention, real backpressure, and real cache-line effects. The limitation is that the queue owns both ordering and storage. That makes it harder to express more advanced topologies such as fanout, multiple consumers, or dependency chains.

## Push vs Try-Push

## Backlog, Capacity, and Drop Policy

## Padding and Cache-Line Effects

## Sequencer Model

The sequencer model separates ordering from storage.
In the MPSC ring, the ring owns both:
```text 
sequence control + payload storage 
```
In the sequencer model, these become separate:
```
sequencer       -> controls sequence numbers and visibility 
ring storage    -> stores payloads at seq & (capacity - 1)
consumer        -> reads only after seq becomes available 
``` 

For the current `SingleProducerSequencer`, the key state is:
```text 
next_   = next sequence the producer may claim 
cursor_ = highest published sequence 
gating_ = highest consumed sequence 
```

A producer flow becomes:
```
seq = next()
ring[seq & mask] = event 
publish(seq)
```

A consumer flow becomes:
```text 
wait_until_available(seq)
event = ring[seq & mask]
mark_consumed(seq) 
```

The important idea is:
```text
claim != publish != consume 
```

Claiming reserves a sequence. Publishing makes it visible. Consuming moves the gating sequence so capacity can be reused.
This is the mental bridge from the current MPSC ring toward a Disruptor-style design.



## How MPSC Relates to Disruptor

The current MPSC ring already uses several Disruptor-like ideas:

- monotonically increasing sequence numbers 
- power-of-two ring indexing
- bounded capacity
- separation between claim and publish 
- per-slot publication state 
- busy-polling on the hot path 

However, the current MPSC ring is still a queue, not a full Disruptor.

Current MPSC model:
```text 
multiple producers -> one ring -> one consumer 
```
A Disruptor-style model can support fanout:
```text 
producer -> ring storage -> consumer A 
                         -> consumer B
                         -> consumer 
```
The key difference is how consumer progress is tracked.
In the current MPSC ring, there is one consumer position:
```text 
tail_
```
In a Disruptor-style design, each consumer may have its own sequence:
```text 
consumer_a_seq
consumer_b_seq
consumer_c_seq
```
The producer must not wrap around and overwrite data that any consumer has not processed yet. Therefore capacity is controlled by the slowest consumer:
```text 
available capacity depends on min(gating_sequences)
```
This is why the sequencer abstraction matters. It gives us a place to model producer claims, publication, and consumer progress without tying those ideas directly to one specific queue implementation.


## What Is Experimental vs Core

Some v0.3 work is part of the engine direction, while some work is intentionally experimental.

Core direction:
- bounded ring buffers
- explicit producer and consumer sequences
- clear claim / publish / consume lifecycle 
- capacity and backpressure behavior 
- latency and throughput measurement 

Experimental work:
- `try_drop` overload policy 
- different ring capacities 
- padded vs cell-padded layouts 
- standalone `Sequence` abstraction
- `SingleProducerSequencer`
- Disruptor-style fanout exploration 

The purpose of the experimental work is not to commit to a final design too early. It is to make concurrency trade-offs visible through small implementations and measurements.

A feature should move from experimental to core only when it has:
- a clear use case in the market data engine 
- correctness tests 
- benchmark data 
- documented trade-offs 

## Next Milestones
