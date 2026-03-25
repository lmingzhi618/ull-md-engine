# **Busy Polling vs Blocking Queue — CPU Profile Analysis**



## **Overview**

This experiment compares CPU cost between:

- **Busy polling** using SpscRing
- **Blocking queue** using mutex + condition_variable

We use gperftools + pprof CPU profiling to understand where CPU time is spent in each mode.

---

# **Experiment Setup**

Benchmark:

```code
busy_vs_blocking_bench
```

Two modes:

```code
busy
blocking
```

Each run:

- producer thread pushes messages
- consumer thread pops messages
- latency recorded via LatencyHist
- profiling duration ≈ 10+ seconds

---

# **Busy Polling Profile**

Top entries:

```code
flat      cum      function
---------------------------------------
78.7%     100%     [libc++.1.dylib]
6.9%      50.6%    run_busy_bench lambda
4.9%      49.3%    run_busy_bench
3.2%      3.2%     atomic_store
2.0%      2.1%     LatencyHist::add
1.7%      1.7%     atomic_load
0.9%      4.1%     SpscRing::try_push
0.7%      1.9%     SpscRing::try_pop
```

## **Observations**

### **1. Busy polling dominated by spin/runtime**

Most CPU samples fall inside:

```code
libc++
thread runtime
spin loop
atomic operations
```

This indicates CPU is continuously burned in busy polling.

---

### **2. SPSC ring is not the bottleneck**

```code
try_push  ~4%
try_pop   ~2%
```

Ring buffer operations are lightweight.

Main cost inside ring:

```code
atomic load
atomic store
```

### **3. Histogram cost is small**

```code
LatencyHist::add ≈ 2%
```

Measurement overhead is minimal.

---

### **Busy Mode Summary**

Busy polling CPU time is dominated by:

- spin loop
- runtime scheduling
- atomic operations

NOT by:

- queue implementation
- histogram logic

---

# **Blocking Queue Profile**

Top entries:

```code
flat      cum      function
---------------------------------------
51.3%              libsystem_kernel
48.6%              libc++
48.1%              unique_lock ctor
40.9%              unique_lock dtor
10.4%              condition_variable wait
```

## **Observations**

### **1. CPU shifts into kernel wait path**

Blocking queue spends time in:

```code
pthread
kernel wait
condition variable
mutex
```

Instead of spin loop.

---

### **2. Mutex lifecycle is very expensive**

Major costs:

```code
unique_lock ctor 48%
unique_lock dtor 40%
```

Blocking overhead includes:

- lock acquire
- condition check
- sleep
- wakeup
- unlock

Not just cv.wait().

---

### **3. condition_variable wait is visible but not dominant**

```code
condition_variable::wait ≈ 10%
```

Most overhead is actually mutex + runtime.

---

### **Blocking Mode Summary**

Blocking queue CPU time dominated by:

- mutex lifecycle
- condition variable
- kernel wait/wakeup
- pthread runtime

---

# **Busy vs Blocking Comparison**

## **Busy Polling**

CPU cost dominated by:

```code
spin loop
atomic operations
runtime
```

Characteristics:

- high CPU usage
- low latency
- minimal synchronization cost

---

## **Blocking Queue**

CPU cost dominated by:

```code
mutex
condition_variable
kernel wait
thread wakeup
```

Characteristics:

- low spin CPU
- higher synchronization overhead
- potential latency increase

---

Characteristics:

- low spin CPU
- higher synchronization overhead
- potential latency increase

---

# **Key Takeaways**

### **1. Busy polling burns CPU**

Busy mode keeps CPU active continuously.

Most time spent in spin/runtime paths.

------

### **2. Blocking shifts cost to synchronization**

Blocking queue avoids spin but pays for:

- mutex
- wakeup
- kernel transitions

------

### **3. SPSC ring is efficient**

Ring buffer is **not** the bottleneck.

Atomic operations dominate ring cost.

------

### **4. Trade-off confirmed**

Busy polling:

- higher CPU
- lower latency

Blocking queue:

- lower spin CPU
- higher synchronization overhead

------

# **Conclusion**

CPU profiling confirms the expected trade-off:

Busy polling is dominated by spin/runtime overhead, while blocking is dominated by mutex, condition-variable, and kernel wait/wakeup cost. The SPSC ring buffer itself is efficient and not a primary bottleneck.

------

# **Reproduce**

Busy profile:

```code
CPUPROFILE=cpu.prof ./busy_vs_blocking_bench busy
pprof -top cpu.prof
pprof -tree cpu.prof
```

Blocking profile:

```code
CPUPROFILE=cpu.prof.blocking ./busy_vs_blocking_bench blocking
pprof -top cpu.prof.blocking
pprof -tree cpu.prof.blocking
```

