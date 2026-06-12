# MPSC Memory Ordering Notes

## Goal 

Document the acquire/release reasoning behind the bounded MPSC ring.

The queue uses a per-slot sequence protocol. `head_` assigns producer positions, while each cell's `seq` controls whether the cell is writable or readable.

## Key Invariants

Each logical position `pos` maps to a physical slot:

```cpp 
Cell &cell = buf_[pos & mask_];
```

A cell moves through these states:

```text 
seq == pos          producer may write this cell 
seq == pos + 1      consumer may read this cell 
seq == pos + cap    cell is recycled for the next producer round 
```

## Producer Publish

Producer writes payload first:

```cpp 
cell.value = v;
```

Then publishes the cell:

```cpp 
cell.seq.store(pos + 1, std::memory_order_release);
```

The release store guarantees that the payload write becomes visible before the consumer observes the published sequence.

## Consumer Readiness Check 

Consumer checks readiness with acquire:

```cpp
if (cell.seq.load(std::memory_order_acquire) != tail_ + 1) {
    return false;
}
```

If the consumer observes `seq == tail_ + 1`, the acquire load pairs with the producer's release store. The consumer can then safely read:

```cpp 
out = cell.value;
```

## Slot Recycling 

After reading the payload, the consumer recycles the cell:

```cpp 
cell.seq.store(tail_ + cap_, std::memory_order_release);
```

A future producer waits for:

```cpp 
cell.seq.load(std::memory_order_acquire) == pos 
```

This prevents the producer from overwriting a slot before the consumer has finished reading the previous value.

## Why head_ Can Use Relaxed Ordering 
head_ only assigns unique logical positions to producers.
For spinning push():
```cpp 
head_.fetch_add(1, std::memory_order_relaxed);
``` 

For non-blocking try_push():
```cpp 
head_.compare_exchange_weak(
  pos,
  pos+1,
  std::memory_order_relaxed,
  std::memory_order_relaxed);
```

These operations need atomicity, but they do not publish payload data. Payload visibility is controlled by each cell's `seq` acquire/release protocol.
Therefore, head_ does not need acquire/release ordering.

## Why tail_ Is Non-Atomic 

The queue has exactly one consumer. Only the consumer thread reads and writes tail_, so tail_ does not need to be atomic. Producers do not inspect tail_; they only inspect per-cell seq.

## Claim vs Publish 

Claiming a position is not the same as publishing a message.
Claim:
```text 
producer reserves a logical position using `head_` 
```
Publish:
```text 
producer finishes writing payload and stores `seq = pos + 1`
```

This separation prevents the consumer from reading partially written payload.

## Summary 

The memory order design is:
```text 
head_ relaxed:
  unique producer position assignment only 

cell.seq release/acquire 
  payload visibility and slot ownership transfer 

tail_ non-atomic:
  single-consumer private state 
```



