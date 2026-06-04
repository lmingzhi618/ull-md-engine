# MPSC Padding Experiment

## Goal

Measure whether cache-line padding improves the MPSC ring under producer contention.

This experiment compares:
- `MpscRing`
- `MpscRingPadded`

The padded version isolates producer-owned `head_` and consumer-owned `tail_` onto separate cache lines.

## Queue Variants

### Plain 

The plain queue stores metadata fields without explicit cache-line separation:
```cpp
std::atomic<std::uint64_t> head_;
std::uint64_t tail_;
```

### Padded 

The padded queue separates head_ and tail_:
```cpp
alignas(kCacheLine) std::atomic<std::uint64_t> head_;
alignas(kCacheLine) std::uint64_t tail_;
```

only head_ and tail_ were padded in this experiment.
Per-cell padding was not tested in this round.

## Benchmark Setup

### Command shape:
```bash 
./scripts/run_mpsc_ring_rel.sh push <producers> 500000 50000 1024 <layout>
```

### Parameters:
- mode: push 
- producers: 4, 8
- messages per producer: 500,000
- warmup messages: 50,000 
- capacity: 1,024 
- layouts: plain, padded

## Results 

### 4 Producers 
| Layout | Throughput msg/s | p50 ns | p99 ns | p999 ns |
|-------:|-----------------:|-------:|-------:|--------:|
|plain	 |3.55M	            |277,950 |382,150 |1,435,300|
|padded	 |3.95M	            |254,75  |299,300 |316,000|

### 8 Producers
| Layout | Throughput msg/s | p50 ns | p99 ns   | p999 ns |
|-------:|-----------------:|-------:|---------:|--------:|
| plain	 |2.70M	            |336,000 |1,842,950	|7,807,700|
| padded |2.88M	            |310,250 |1,566,500	|7,690,100|

## Observations 

Head/Tail padding improves throughput and reduces latency, especially at p99.
For 4 producers, padded layout improved throughput by roughly 11% and reduced p99 latency by roughly 22%.
For 8 producers, padded layout improved throughput by roughly 7% and reduced p99 latency by roughly 15%.
The p999 improvement is large for 4 producers, but much smaller for 8 prodcuers.

## Interpretation

The results suggest that false sharing between prodcuer-side head_ and consumer-side tail_ was present in the plain layout.
Separating head_ and tail_ reduces producer-consumer cache-line interference, but it does not remove the main MPSC contention point.

The dominant remaining costs are likely:
- producer-prodcuer contention on the shared head_
- single-consumer drain capacity 
- possible cache-line interaction among adjacent cells
This means head/tail padding is useful, but not sufficient to make the MPSC ring scale linearly with producer count.

## Next Questions
- Would per-cell padding reduce slot-level false sharing?
- Would per-cell padding hurt cache locality enough to offset gains?
- How does padded layout behave with larger capacities?
- How does padded layout behave in try_drop mode?

