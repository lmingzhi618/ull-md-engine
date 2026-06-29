# Disruptor Component Map

## Goal 

This note explains how the current v0.3 Disruptor-style components fit together.
The goal is not to describe a full Disruptor implementation. The goal is to map the current experimental components to their responsibilities.

## Components 

### Ring Storage

Ring storage owns event data. It is indexed by logical sequence numbers:
```text
slot = seq & (capacity - 1)
```
It does not decide whether a sequence is available or whether a slot is safe to reuse.

### SingleProducerSequencer

`SingleProducerSequencer` controls producer-side sequencing.

Responsibilities:
- claim the next logical sequence
- publish the highest visible sequence 
- track remaining capacity 
- map sequence numbers to ring slots 
- use gating progress to prevent unsafe slot reuse 

Important state:
```text 
next_   = next sequence the producer may claim 
cursor_ = highest published sequence 
gating_ = internal single-consumer progress 
``` 
With external GatingSequences, capacity is controlled by the slowest consumer.

Consumer-side waiting is now modeled by `SequenceBarrier`. The sequence may still expose small convenience helpers for tests, but new consumer-facing code should wait through a barrier. 

### GatingSequences

`GatingSequences` tracks consumer progress for fanout.
Each consumer has its own sequence:
```text 
consumer 0 -> seq
consumer 1 -> seq
consumer 2 -> seq
```
The producer must respect the slowest consumer:
```text 
slowest_consumer_sequence = min(gating_sequences)
```
This prevents the producer from reusing a slot before every consumer has finished reading the old event in that slot.

### SequenceBarrier

`SequenceBarrier` is the consumer-side visibility boundary. A consumer uses it before reading ring storage:
```text 
barrier.wait_until_available(seq)
event = ring[seq & mask]
```
The barrier does not own data. It asks the sequencer whether a sequence has been published and is viaible.
Conceptually:
```text 
SequenceBarrier answers:
    "May this consumer read seq now?"
```

### WaitStrategy 

`WaitStrategy controls how a consumer waits when a sequence is not available yet.
Current implementation:
```text 
BusySpinWaitStrategy 
```
For busy spin, idle() intentionally does nothing:
```text 
while seq is not available:
    wait_strategy.idle()
```
This keeps the consumer thread active and avoids scheduler wakeup latency, at the cost of CPU usage.

`SequenceBarrier` is templated on the wait strategy type, so the wait behavior is selected at compile time rather than through virtual dispatch.

## Event Flow 

A single event goes through this path:
```text 
Producer 
  |
  | claim 
  v 
SingleProducerSequencer.next()
  |
  | returns seq 
  v 
Ring storage 
  |
  | ring[seq & mask] = event 
  v 
SingleProducerSequencer.publish(seq) 
  |
  | cursor advances 
  v 
SequenceBarrier.wait_until_available(seq) 
  |
  | if not visible, call WaitStrategy.idle() 
  v 
Consumer reads ring[seq & mask] 
  |
  | consumer progress 
  v
GatingSequences.mark_consumed(consumer_id, seq) 
```

## Responsibility Split 

```text 
Sequencer:
  producer ordering, publication, capacity 

Ring Storage:
  event payload storage 

SequenceBarrier:
  consumer visibility boundary 

WaitStrategy:
  waiting behavior 

GatingSequences:
  consumer progress and fanout backpressure 
```

### Why This Split Matters

Without  this split, a queue tends to combine several concerns:
```text 
claim + storage + publish + wait + consume 
```
That is simpler at first, but harder to extend.
The current split makes it easier to experiment with:
- multiple consumers 
- different wait strategies 
- fanout backpressure 
- future dependency chains 
- future multi-producer publication tracking 

### Current Limitations 

The current model is still experimental.

Limitations:
- `SingleProducerSequencer` assumes in-order publication.
- A single cursor is not enough for out-of-order multi-producer publish.
- `BusySpinWaitStrategy` is the only implemented wait strategy.
- Fanout is demonstrated by tests, not integrated into a full pipeline.
- Ring storage is still manually represented in tests.


