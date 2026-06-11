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

## Try-Drop Padding Results 

The padded layouts were also compared in `try_drop` mode.
In this mode, producers do not wait for capacity. If the queue is full, `try_push` returns `false` and the message is counted as dropped.

Command shape:
```bash 
./scripts/run_mpsc_ring_rel.sh try_drop <producers> 500000 50000 1024 <layout> 
``` 

### 4 Producers: Try-Drop 

| Layout      | Published | Dropped | Throughput msg/s | p50 ns | p99 ns | p999 ns |
|------------:|----------:|--------:|-----------------:|-------:|-------:|--------:|
| padded	  |455,771	  |1,544,229|10.52M            | 427,150| 528,300|	537,450|
| cell_padded |482,204	  |1,517,796|11.26M	           | 398,500| 536,450|	544,250|

### 8 Producers: Try-Drop 

| Layout      | Published | Dropped   | Throughput msg/s | p50 ns   | p99 ns   | p999 ns   |
|------------:|----------:|----------:|-----------------:|---------:|---------:|----------:|
| padded	  | 335,521	  | 3,664,479 | 11.54M	         |1,264,750	|1,376,750 |1,425,200  |
| cell_padded | 340,522	  | 3,659,478 | 11.41M	         |1,308,750	|1,751,750 |1,817,750  |

### Try-Drop Padding Interpretation 

In `try_drop` mode, per-cell padding is less clearly beneficial than in push mode.

For 4 producers, `cell_padded` publishes more messages and improves p50 latency, but p99 and p999 are roughly flat or slightly worse.

For 8 producers, `cell_padded` publishes slightly more messages, but throughput and tail latency are worse than the head/tail padded layout.

This suggests that when producers do not wait for queue capacity, the benefit from reducing slot-level false sharing is smaller. At higher producer counts, the larger memory footprint of per-cell padding may offset its cache-line isolation benefit. 


## Overall Observations 

Head/Tail padding improves throughput and reduces latency moderately, especially at p99.
For 4 producers, padded layout improved throughput by roughly 11% and reduced p99 latency by roughly 22%.
For 8 producers, padded layout improved throughput by roughly 7% and reduced p99 latency by roughly 15%.
The p999 improvement is large for 4 producers, but much smaller for 8 producers.
Per-cell padding shows a stronger improvement for 4 producers, but the 8-producer p999 result shows that more padding does not always improve extreme tail latency.
In `try_drop` mode, per-cell padding is less consistently beneficial: it helps some 4-producer metrics, but worsens 8-producer tail latency.

## Overall Interpretation

The results suggest that false sharing between producer-side head_ and consumer-side tail_ was present in the plain layout.
Separating head_ and tail_ reduces producer-consumer cache-line interference, but it does not remove the main MPSC contention point.

The dominant remaining costs are likely:
- producer-producer contention on the shared head_
- single-consumer drain capacity 
- possible cache-line interaction among adjacent cells
This means head/tail padding is useful, but not sufficient to make the MPSC ring scale linearly with producer count.

The remaining scalability limit is still producer contention and the single-consumer drain point. Padding helps reduce cache-line interference, but it does not change the fundamental MPSC topology.

## Current Conclusion 

Padding helps, but the best layout depends on the workload.

For `push` mode, `cell_padded` is the strongest layout in the tested 4-producer case and improves throughput, p50, and p99 in the 8-producer case. However, the 8-producer p999 result gets worse, which shows that per-cell padding does not universally improve extreme tail latency.

For `try_drop` mode, `cell_padded` is less consistently beneficial. It improves some 4-producer metrics, but worsens 8-producer tail latency compared with head/tail padding only.

The practical takeway is:
```text 
head/tail padding is a low-cost improvement;
per-cell padding is workload-sensitive and should be benchmarked before being treated as the default layout.
```
For now, MpscRingPadded is the safer default experiment variant, while MpscRingCellPadded remains useful for studying slot-level false sharing. 


## Next Questions
- How stable are `padded` vs `cell_padded` results across repeated runs?
- How does `cell_padded` behave with larger capacities?
- Can shared `head_` contention be reduced with a different queue design?
