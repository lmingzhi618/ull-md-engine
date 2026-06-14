# Disruptor-Style Sequence Notes

## Goal

Explore how the current MPSC ring relates to a Disruptor-style design.

The current `MpscRing` already uses several Disruptor-like ideas:

- monotonically increasing sequence numbers
- power-of-two ring indexing 
- per-slot publication state 
- separation between claim and publish 
- busy-polling consumer path 

The next step is to understand what is still missing.

## Current MPSC Model

Current producer flow:

```text
claim position from `head_`
wait for target cell to become reusable
write payload 
publish cell with `seq == pos + 1`
```

Current consumer flow:

```text 
check next `tail_`
read only if the target cell is published
recycle cell for the next producer round
advance `tail_`
```

This is a bounded MPSC queue with one consumer.

## What a Disruptor Adds

A Disruptor-style design separates the queue into more explicit roles:
- sequencer: assigns producer sequences 
- ring buffers: stores events 
- cursor: tracks highest published sequence 
- gating sequences: track consumer progress 
- wait strategy: controls how producers/consumers wait 
- consumer graph: supports fanout and dependency chains 

## Key Difference

The current MPSC queue has one consumer and one implicit consumption position:
```text 
tail_
```

A Disruptor-style design may have multiple consumers, each with its own sequence:
```text 
consumer_a_seq
consumer_b_seq
consumer_c_seq
```

Producers must not wrap around and overwrite slots still needed by the slowest consumer.

This makes capacity control depend on:
```text
min(gating_sequences)
```
not just a single tail_.

## Why This Matters 

For market data, different downstream paths may want the same input:
- order book builder 
- strategy signal path 
- recorder / replay writer 
- monitoring / metrics path 

A single-consumer MPSC queue forces one consumer pipeline. A Disruptor-style topology can model fanout while keeping sequence-based backpressure explicit.

## Possible Evolution Path 

Step 1:
Keep the current single-consumer MPSC queue, but document the roles:
- `head_` as producer claim sequence 
- per-cell `seq` as publication state 
- `tail_` as consumer gating sequence 

Step 2:
Introduce a named `Sequence` abstraction:
```cpp 
struct Sequence {
    std::atomic<std::uint64_t> value;
};
```

A minimal `Sequence` wrapper now exists in:
```text 
include/ull/core/sequence.h
```
It is cache-line aligned and wraps an atomic uint64_t.
It also exposes `compare_exchange_weak()` so future sequencer experiments can use CAS-based sequence claiming.

Step 3:
Experiment with one producer cursor and one consumer gating sequence.
A minimal `SingleProducerSequencer` prototype now exists in:
```text 
include/ull/core/single_producer_sequence.h
``` 
It separates sequence claiming from publication:
```text 
next() claims a sequence
publish() marks it visible through the cursor 
```

The sequencer also exposes `remaining_capacity()` so tests and future experiments can observe how far the producer may advance before hitting the consumer gating sequence.
The sequencer now exposes two claim styles:
```text 
try_next() returns false when there is no remaining capacity 
next() spins until capacity becomes available 
```
This mirrors the queue API distinction between non-blocking try_push() and blocking/spinning push().

Consumer-side availability is represented by:
```text 
is_available(seq)
```
For the single-producer prototype, a sequence is available when:
```text 
seq <= cursor 
```
because publication happens in producer order.

A blocking consumer-side wait is represented by:
```text 
wait_until_available(seq)
```

Consumer progress is reported with:
```text 
mark_consumed(seq);
```
This advances the gating sequence and allows the producer to reuse capacity behind that point.

A small test now demonstrates the sequencer controlling a fixed-size ring:
```text
next() -> write ring slot -> publish() -> wait_until_available() -> read ring slot -> mark_consumed() 
```
This keeps storage separate from sequence control.

Step 4:
Explore multiple consumer gating sequences and publish barriers.

## Open Questions
- Can the current per-slot sequence protocol extend cleanly to fanout?
- Should publication use per-slot `seq` or a separate cursor?
- How should slow consumers affect producer progress?
- Which wait strategies make sense for each consumer type?
- Which market-data paths require lossless delivery?


