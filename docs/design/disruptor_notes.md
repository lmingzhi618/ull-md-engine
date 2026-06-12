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

A Disruptor-style design may have muliple consumers, each with its own sequence:
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
    std::aotmic<std::uint64_t> value;
};
```

Step 3:
Experiment with one producer cursor and one consumer gating sequence.

Step 4:
Explore multiple consumer gating sequences and publish barriers.

## Open Questions
- Can the current per-slot sequence protocol extend cleanly to fanout?
- Should publication use per-slot `seq` or a separate cursor?
- How should slow consumers affect producer progress?
- Which wait strategies make sense for each consumer type?
- Which market-data paths require lossless delivery?


