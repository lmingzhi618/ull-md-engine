# Thread Affinity Experiment — Latency Impact

## 1. Machine / Environment

- **Machine**: Apple MacBook Pro (M2 Pro)
- **CPU Architecture**: ARM64 (heterogeneous cores: performance + efficiency)
- **OS**: macOS
- **Compiler**: Clang (C++20)
- **Build Type**: Release (`-O3`)
- **Threads**:
  - 1 Producer
  - 1 Consumer
- **Queue Model**: Lock-free SPSC ring buffer (busy polling)
- **Affinity Mechanism**:
  - macOS `THREAD_AFFINITY_POLICY` (best-effort hint, not strict pinning)

---

## 2. Benchmark Design

We evaluate the effect of thread affinity on latency by running the same producer–consumer pipeline under three configurations:

### (1) Default

- No affinity control
- Threads scheduled freely by OS

---

### (2) Same Affinity Group (`same`)

- Producer and consumer assigned the same affinity tag
- Encourages co-location on the same scheduling group

---

### (3) Split Affinity (`split`)

- Producer and consumer assigned different affinity tags
- Encourages separation across cores

---

### Common Configuration

- `N = 2,000,000` messages

- Warmup: `200,000`

- Histogram:
  - Unit: nanoseconds
  - Bucket size: 50 ns
  - Max range: 20 ms
  
- Latency definition:

  latency = consumer_receive_time - producer_send_time

  ---

  ## 3. Results Summary

  Across repeated runs:

  ### Default

  - p50: ~2.5–2.9 ms
  - p99: ~3.3–7.6 ms
  - p999: ~3.3–7.6 ms
  - **High run-to-run variance**

  ---

  ### Same Affinity

  - p50: ~2.5–3.0 ms
  - p99: ~5.0–7.9 ms
  - p999: ~5.0–7.9 ms
  - **Consistently worse tail latency**

  ---

  ### Split Affinity

  - p50: ~2.5–3.0 ms
  - p99: ~3.9–7.7 ms
  - p999: ~4.0–7.7 ms
  - **Occasional improvement, but not consistent**

  ---

  ## 4. Interpretation

  ### 4.1 Same Affinity Increases Contention

  Assigning both threads to the same affinity group tends to:

  - Increase scheduling contention
  - Introduce time-slicing behavior
  - Degrade tail latency (p99 / p999)

  ---

  ### 4.2 Default Scheduling Is Highly Variable

  Without affinity:

  - OS freely migrates threads
  - Execution depends on system load and scheduler decisions
  - Leads to significant run-to-run variability

  ---

  ### 4.3 Split Affinity Is Not Consistently Beneficial

  Although splitting threads across affinity groups sometimes improves latency:

  - The effect is inconsistent
  - Improvements are not reliably reproducible

  ---

  ### 4.4 Platform Limitation: macOS Affinity Semantics

  The key factor:

  > macOS `THREAD_AFFINITY_POLICY` is a **hint**, not strict CPU pinning.

  This differs from Linux:

  | Platform                         | Affinity Strength     |
  | -------------------------------- | --------------------- |
  | Linux (`sched_setaffinity`)      | Strong (hard binding) |
  | macOS (`THREAD_AFFINITY_POLICY`) | Weak (scheduler hint) |

  As a result:

  - Threads may still migrate across cores
  - Core placement is not deterministic
  - Affinity effects are diluted

  ---

  ### 4.5 Heterogeneous Core Effects (M2 Pro)

  The M2 Pro uses:

  - Performance cores (P-cores)
  - Efficiency cores (E-cores)

  Thread placement may vary between runs:

  - P-core ↔ E-core migration
  - Mixed scheduling scenarios

  This contributes to:

  - Tail latency spikes
  - Increased variance

  ---

  ## 5. Takeaway

  ### Key Findings

  - Thread affinity **does affect latency**, especially tail latency
  - Co-locating producer and consumer (**same affinity**) is consistently harmful
  - Splitting threads (**split affinity**) does not guarantee improvement on macOS

  ---

  ### Engineering Insight

  > Affinity control is only as strong as the underlying OS support.

  - Weak affinity → weak determinism improvement
  - Strong affinity (e.g., Linux) → predictable gains

  ---

  ### Practical Implication

  - On macOS, affinity tuning has limited impact for latency-sensitive workloads
  - For production-grade low-latency systems:
    - Prefer platforms with **strong CPU pinning support**
    - Combine affinity with:
      - NUMA awareness
      - cache locality
      - polling strategies

  ---

  ### Conclusion

  > Thread affinity improves latency determinism only when the platform provides strong guarantees. On macOS/M2 Pro, affinity hints are insufficient to deliver consistent improvements, though poor affinity choices (co-location) can still degrade performance.

  ---
