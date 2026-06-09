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

Only head_ and tail_ were padded in this experiment.
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
- layouts: plain, padded, cell_padded

## Results 

### 4 Producers 
| Layout | Throughput msg/s | p50 ns | p99 ns | p999 ns |
|-------:|-----------------:|-------:|-------:|--------:|
|plain	 |3.55M	            |277,950 |382,150 |1,435,300|
|padded	 |3.95M	            |254,750 |299,300 |316,000|

### 8 Producers
| Layout | Throughput msg/s | p50 ns | p99 ns   | p999 ns |
|-------:|-----------------:|-------:|---------:|--------:|
| plain	 |2.70M	            |336,000 |1,842,950	|7,807,700|
| padded |2.88M	            |310,250 |1,566,500	|7,690,100|

## Per-Cell Padding Results 

A third layout was added after the head/tail padding experiment:

- `plain` 
- `padded`
- `cell_padded`

The `cell_padded` layout keeps `head_` and `tail_` cache-line separated, and also aligns each ring cell to a cache line.

This tests whether adjacent cells sharing a cache line create slot-level false sharing.

Command shape:

```bash 
./scripts/run_mpsc_ring_rel.sh push <producers> 500000 50000 1024 <layout>
```

### 4 Producers: Padded vs Cell Padded 

| Layout      | Throughput msg/s | p50 ns  | p99 ns    | p999 ns   |
|------------:|-----------------:|--------:|----------:|----------:|
| padded	  | 3.32M	         | 268,100 | 1,791,300 | 7,192,050 |
| cell_padded | 4.71M	         | 205,900 | 256,750   | 313,000   |

### 8 Producers: Padded vs Cell Padded 

| Layout      | Throughput msg/s | p50 ns  | p99 ns    | p999 ns    |
|------------:|-----------------:|--------:|----------:|-----------:|
| padded	  | 2.64M	         | 306,850 | 2,759,600 | 11,027,250 |
| cell_padded | 3.01M	         | 252,250 | 2,414,000 | 13,761,500 |

### Per-Cell Padding Observations 

Per-cell padding significantly improves 4-producers performance. Throughput increases from about 3.32M msg/s to 4.71M msg/s, and p99 latency drops from about 1.79ms to 257us.

For 8 producers, `cell_padded` also improves throughput, p50, and p99 latency. However, p999 latency becomes worse than the head/tail padded layout.

This suggests that slot-level false sharing matters, but per-cell padding is not a free win for every tail percentile.

### Per-Cell Padding Interpretation 

The 4-producer result strongly suggests that adjacent cells sharing a cache line caused measurable producer/consumer interference.

By aligning each cell to its own cache line, producers and the consumer touch fewer unrelated slots on the same cache line. This improves normal operation and p99 latency.

At 8 producers, producer contention is higher. Even though per-cell padding improves throughput and p99 latency, the extreme tail gets worse. Possible reasons include:
- continued contention on the shared head_ 
- larger memory footprint from one-cache-line-per-cell layout 
- additional cache or TLB pressure 
- scheduler effects at higher thread counts 
- rare slot wait or cache miss events becoming more expensive

The main lesson is that padding reduces some forms of false sharing, but it can also increase memory footprint. In low-latency systems, improving p50 or p99 does not guarantee an improvement at p999.

## Overall Observations 

Head/Tail padding improves throughput and reduces latency moderately, especially at p99.
For 4 producers, padded layout improved throughput by roughly 11% and reduced p99 latency by roughly 22%.
For 8 producers, padded layout improved throughput by roughly 7% and reduced p99 latency by roughly 15%.
The p999 improvement is large for 4 producers, but much smaller for 8 producers.
Per-cell padding shows a stronger improvement for 4 producers, but the 8-producer p999 result shows that more padding does not always improve extreme tail latency.

## Overall Interpretation

The results suggest that false sharing between producer-side head_ and consumer-side tail_ was present in the plain layout.
Separating head_ and tail_ reduces producer-consumer cache-line interference, but it does not remove the main MPSC contention point.

The dominant remaining costs are likely:
- producer-producer contention on the shared head_
- single-consumer drain capacity 
- possible cache-line interaction among adjacent cells
This means head/tail padding is useful, but not sufficient to make the MPSC ring scale linearly with producer count.

The remaining scalability limit is still producer contention and the single-consumer drain point. Padding helps reduce cache-line interference, but it does not change the fundamental MPSC topology.

## Next Questions
- How stable are `padded` vs `cell_padded` results across repeated runs?
- How does `cell_padded` behave with larger capacities?
- How does `cell_padded` behave in `try_drop` mode?
- Can shared `head_` contention be reduced with a different queue design?

