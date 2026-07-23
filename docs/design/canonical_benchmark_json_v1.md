# Canonical Benchmark JSON v1

## 1. Problem 

Current benchmark outputs are optimized for human reading:

```text 
throughput_msg_per_sec=...
p50_ns=...
p99_ns=...
p999_ns=...
```

This is useful during manual experiments, but it is not enough for the next project phase.

v0.4 requires benchmark results that can support:
- repeated trial comparison 
- regression detection 
- plotting 
- system metadata correlation 
- future `perf stat` correlation 
- future automated analysis

The problem being studied is:
```text 
What is the smallest userful machine-readable benchmark result format?
```
This document defines a minimal JSON result shape for early v0.4 work.

---

# 2. Design Goal 

The JSON format should make benchmark runs comparable without turning the project into a premature benchmark framework.

The format should be:
- explicit 
- stable enough for scripts 
- easy to emit from C++
- easy to inspect by humans 
- extensible for Linux profiling and AI-infrastructure experiments 

The first implementation should target one benchmark only.

Recommended first target:
```text 
sp_fanout_bench
```

Reason:
- it already has multiple configuration fields 
- it has per-consumer metrics
- it exposes scheduler and affinity effects 
- it is a good bridge from v0.3 to v0.4

---

## 3. Non-Goals 

This v1 design does not implement:
- a full benchmar framework 
- automatic result comparison 
- automatic plotting 
- `perf stat` integestion 
- system topology detection 
- source-control metadata collection 
- schema validation tooling 
- JSON output for every benchmark  

Those can come later after one benchmark emits a stable result.

## 4. Minimal JSON Example 

Example for `sp_fanout_bench`:
```json
{
  "schema_version": "1.0",
  "benchmark": {
    "name": "sp_fanout_bench",
    "category": "concurrency",
    "version": "0.1"
  },
  "configuration": {
    "consumers": 4,
    "messages": 1000000,
    "capacity": 1024,
    "affinity": "default",
    "warmup": 50000
  },
  "measurement": {
    "measured_messages": 950000,
    "elapsed_ns": 182948208,
    "throughput_msg_per_sec": 5466030.0
  },
  "latency": {
    "unit": "ns",
    "bucket_ns": 50,
    "max_ns": 20000000,
    "consumers": [
      {
        "consumer": 0,
        "count": 950000,
        "p50_ns": 250,
        "p99_ns": 292200,
        "p999_ns": 337000
      },
      {
        "consumer": 1,
        "count": 950000,
        "p50_ns": 250,
        "p99_ns": 183750,
        "p999_ns": 261750
      }
    ]
  },
  "notes": [
    "macOS affinity is best-effort and not strict CPU pinning"
  ]
}
```

## 5. Field Meaning 

`schema_version`
Identifiers the JSON result format.
This is separate from:
- project version 
- benchmark version 
- git commit 
- release tag 

The first version is:
```json
"schema_version": "1.0"
```

`benchmark`
Identifies the workload.
Required fields:
- `name`
- `category`

Recommended fields:
- `version`

Example:
```json 
"benchmark": {
  "name": "sp_fanout_bench",
  "category": "concurrency",
  "version": "0.1"
}
```

The benchmark version should change when workload semantics change.

`configuration`
Captures the independent variables and controlled parameters.
For `sp_fanout_bench`:
```json 
"configuration": {
    "consumers": 4, 
    "messages": 1000000,
    "capacity": 1024, 
    "affinity": "default",
    "warmup": 
}
```

This section should answer:
```text 
What did this benchmark run actually test?
```

`measurement` 
Captures run-level results.
Example:
```json 
"measurement": {
    "measured_messages": 950000,
    "elapsed_ns": 182943208,
    "throughput_msg_per_sec": 5466030.0
}
```

This section should answer:
```text 
How much work was measured, how long did it take, and what was the resulting throughput?
```

`latency`
Captures latency distribution data.
For single-consumer benchmarks, this may contain one result.
For fanout benchmarks, it should contain one entry per consumer.

Example:
```json
"latency": {
   "unit": "ns",
   "bucket_ns": 50,
   "max_ns": 20000000,
   "consumers": [
        {
            "consumer": 0,
            "count": 950000,
            "p50_ns": 250,
            "p99_ns": 292200,
            "p999_ns": 337000
        }
   ]
}
```

The initial v1 shape records percentiles rather than the full histogram.
Full histogram export can be added later if a real analysis need appears.

`notes`
Human-readable context.

Example:
```json
"notes": [
    "macOS affinity is best-effort and not strict CPU pinning"
]
```

Notes are not intented for machine comparison.
They are for preserving important experiment context.

---

## 6. First Implementation Target 

The first benchmark to support JSON should be:
```text 
sp_fanout_bench 
```

Proposed CLI:
```bash 
./build/rel/sp_fanout_bench 4 1000000 1024 default 50000 --json 
```

Current behavior:
- JSON output is the default for `sp_fanout_bench`
- `--json` explicitly selects JSON output 
- `--text` keeps the human-readable output for manual debugging 

This keeps the first implementation small.

---

## 7. What Should Stay Constant 

For the first JSON implementation:
- benchmark workload semantics 
- latency histogram behavior 
- warmup behavior 
- throughput calculation 
- existing human-readable output 

Ths JSON path should serialize the same measured values already reported by the text path.
It should not change benchmark behavior.

---

## 8. What Should Vari 

Only the output format should vary:
```text 
default -> machine-readable JSON 
--json  -> machine-readable JSON 
--text  -> machine-readable text 
```

The benchmark should produce the same underlying measurements in both modes.

---

## 9. Metrics Required in V1

Required:
- `elapsed_ns`
- `throughput_msg_per_sec`
- `measured_messages`
- `count`
- `p50_ns`
- `p99_ns`
- `p999_ns`

Required configuration 
- benchmark name 
- messages
- warmup 
- capacity 
- affinity mode 
- consumer count 

Optional later:
- min 
- max 
- average 
- full histogram buckets 
- trial index 
- trial count 
- git commit 
- compiler version 
- CPU model 
- OS version 
- `perf stat` counters 

--- 

## 10. Results That Support the Design 

The v1 design is successful if:
- `sp_fanout_bench --json` emits valid JSON 
- the JSON contains all values currently printed in text mode 
- a simple script can parse throughput and p99 latency 
- adding JSON does not change benchmark measurements 
- text output remains convention for manual runs 

---

## 11. Results That Refute the Design 

The design is too small if:
- comparison scripts immediately need fields not preset in v1 
- per-consumer results cannot be represented cleanly 
- future `perf stat` output has no nature place to attach 

The design is too large if:
- implementing `--json` requires a generic framework 
- every benchmark must be refactored before one benchmark can emit JSON 
- JSON support changes benchmark timing behavior 

--- 

## 12. Common Mistakes to Avoid 

Avoid mixing human formatting into JSON.

Bad:
```json
"throughput": "5.46M msg/s"
```
Good:
```json 
"throughput_msg_per_sec": 5466030.0
```

Avoid hiding units:
Bad:
```json 
"elapsed": 182948208 
```
Good:
```json 
"elapsed_ns": 182948208 
```

Avoid pretending metadata exists when it has not been collected.
Bad:
```json 
"cpu_model": "unkonwn"
```
Better:
```json 
"system": null 
```
or omit the field in v1.

Avoid implementing a framework before two benchmarks need the same abstraction.
The first goal is one useful JSON-producing benchmark, not a benchmark platform rewrite.

---

## 13. Next Implementation Step 

Add `--json` support to `sp_fanout_bench` 
Small Implementation plan:
1. Parse optional `--json`.
2. Store measured values in local variables as today.
3. Add a small JSON printing function inside `sp_fanout_bench_main.cpp`.
4. Keep text output as the default. 
5. Do not introduce shared JSON infrastructure yet.

After that works, run:
```bash 
./build/rel/sp_fanout_bench 4 1000000 1024 default 50000 --json 
```
Then verify:
```bash 
./build/rel/sp_fanout_bench 4 1000000 1024 default 50000 --json | jq . 
```

