# Canonical Benchmark Result Schema

## 1. Purpose

This document defines the canonical machine-readable result format for `ull-md-engine`.

From v0.4 onward, new benchmarks should support JSON output based on this model.

The format exists to support:

* reproducibility
* benchmark comparison
* regression analysis
* chart generation
* artifact linking
* future bottleneck analysis

Human-readable console output may remain.

JSON is the canonical machine-readable result.

---

# 2. Design Principles

The result model should be:

* explicit
* versioned
* extensible
* workload-independent
* unit-safe
* honest about unavailable information

The schema should not require every benchmark to populate every possible field.

Unavailable information should be:

* omitted
* or explicitly `null`

Never fabricate values.

---

# 3. Top-Level Structure

Recommended top-level structure:

```json
{
  "schema_version": "1.0",
  "run": {},
  "benchmark": {},
  "build": {},
  "system": {},
  "configuration": {},
  "measurement": {},
  "metrics": {},
  "artifacts": [],
  "notes": []
}
```

---

# 4. `schema_version`

Example:

```json
{
  "schema_version": "1.0"
}
```

Purpose:

* identify incompatible result formats
* support future migration
* allow analysis tools to validate inputs

The result schema version is separate from:

* project version
* benchmark version
* git commit

---

# 5. `run`

The `run` object identifies one benchmark execution.

Example:

```json
{
  "run": {
    "id": "2026-07-06T23-12-04Z-mpsc-bench-a1b2c3",
    "timestamp_utc": "2026-07-06T23:12:04Z",
    "trial_index": 1,
    "trial_count": 5
  }
}
```

Recommended fields:

| Field           | Type    | Meaning                         |
| --------------- | ------- | ------------------------------- |
| `id`            | string  | unique or locally unique run ID |
| `timestamp_utc` | string  | ISO-8601 UTC timestamp          |
| `trial_index`   | integer | current trial number            |
| `trial_count`   | integer | planned number of trials        |

Optional future fields:

* experiment ID
* comparison group
* operator-provided label

---

# 6. `benchmark`

Example:

```json
{
  "benchmark": {
    "name": "mpsc_bench",
    "version": "0.3",
    "category": "concurrency",
    "description": "Bounded MPSC contention benchmark"
  }
}
```

Recommended fields:

| Field         | Required    | Meaning                                       |
| ------------- | ----------- | --------------------------------------------- |
| `name`        | yes         | stable executable/workload identity           |
| `version`     | recommended | logical workload version                      |
| `category`    | optional    | concurrency, memory, gpu, communication, etc. |
| `description` | optional    | short human-readable description              |

The benchmark version should change when workload semantics materially change.

---

# 7. `source`

Source-control identity may be represented inside `run` or as a separate object.

Recommended:

```json
{
  "source": {
    "git_commit": "0123456789abcdef",
    "git_commit_short": "0123456",
    "git_branch": "main",
    "git_dirty": false
  }
}
```

Rules:

* do not fabricate commit IDs
* dirty state should be explicit when available
* benchmark comparison should warn when source states differ

---

# 8. `build`

Example:

```json
{
  "build": {
    "type": "Release",
    "compiler": "clang",
    "compiler_version": "18.1.3",
    "language_standard": "C++20",
    "compiler_flags": [
      "-O3",
      "-march=native"
    ],
    "lto": false,
    "pgo": false
  }
}
```

Recommended fields:

* build type
* compiler
* compiler version
* language standard
* compiler flags
* LTO status
* PGO status

Future GPU fields may include:

* CUDA compiler version
* architecture targets

---

# 9. `system`

The system object describes execution hardware and software.

Recommended hierarchy:

```json
{
  "system": {
    "host": {},
    "os": {},
    "cpu": {},
    "memory": {},
    "numa": {},
    "gpu": [],
    "network": {}
  }
}
```

---

## 9.1 `system.host`

Example:

```json
{
  "host": {
    "hostname": "perf-node-01"
  }
}
```

The hostname may be omitted for privacy.

Future public results may prefer an anonymized host ID.

---

## 9.2 `system.os`

Example:

```json
{
  "os": {
    "name": "Ubuntu",
    "version": "24.04",
    "kernel": "6.8.0"
  }
}
```

---

## 9.3 `system.cpu`

Example:

```json
{
  "cpu": {
    "vendor": "AMD",
    "model": "AMD EPYC ...",
    "architecture": "x86_64",
    "logical_cpus": 64,
    "physical_cores": 32,
    "sockets": 1,
    "smt_enabled": true,
    "cache_line_bytes": 64
  }
}
```

Optional future fields:

* L1 size
* L2 size
* LLC size
* base frequency
* governor
* turbo status

Only include values that can be collected reliably.

---

## 9.4 `system.memory`

Example:

```json
{
  "memory": {
    "total_bytes": 137438953472,
    "page_size_bytes": 4096,
    "huge_page_size_bytes": 2097152
  }
}
```

---

## 9.5 `system.numa`

Example:

```json
{
  "numa": {
    "available": true,
    "node_count": 2,
    "cpu_nodes": {
      "0": [0, 1, 2, 3],
      "1": [32, 33, 34, 35]
    }
  }
}
```

Large topology data may instead be stored as an external artifact.

---

## 9.6 `system.gpu`

Use an array because multiple devices may participate.

Example:

```json
{
  "gpu": [
    {
      "device_id": 0,
      "name": "NVIDIA GPU",
      "uuid": null,
      "memory_bytes": 85899345920,
      "driver_version": "xxx",
      "cuda_version": "xx.x"
    }
  ]
}
```

Possible fields:

* device ID
* model
* UUID
* memory
* driver
* CUDA version
* compute capability

Do not expose sensitive infrastructure identifiers unnecessarily.

---

## 9.7 `system.network`

For later distributed work:

```json
{
  "network": {
    "interfaces": [],
    "rdma_available": null,
    "transport": null
  }
}
```

This section should remain minimal until v0.9 or v1.0 provides real requirements.

---

# 10. `configuration`

This section records benchmark-controlled variables.

Example:

```json
{
  "configuration": {
    "messages": 10000000,
    "warmup_messages": 100000,
    "queue_capacity": 4096,
    "producer_count": 4,
    "consumer_count": 1,
    "mode": "try_drop",
    "layout": "padded"
  }
}
```

The configuration object is intentionally benchmark-specific.

Rules:

* values should be explicit
* names should be stable
* units should appear in field names when ambiguity exists

Good:

```json
{
  "duration_ns": 1000000
}
```

Bad:

```json
{
  "duration": 1000000
}
```

unless the unit is defined elsewhere unambiguously.

---

# 11. `placement`

CPU, memory, and GPU placement should be explicit when relevant.

Example:

```json
{
  "placement": {
    "cpu_affinity": {
      "producer_cpus": [2, 4],
      "consumer_cpus": [6],
      "policy": "explicit"
    },
    "numa": {
      "cpu_node": 0,
      "memory_node": 0,
      "memory_policy": "bind"
    },
    "gpu_devices": [0, 1]
  }
}
```

Possible CPU placement policies:

* `unrestricted`
* `explicit`
* `same_core`
* `smt_siblings`
* `same_numa`
* `cross_numa`

The exact label should reflect what the benchmark really configured.

---

# 12. `measurement`

This section describes how results were collected.

Example:

```json
{
  "measurement": {
    "warmup": {
      "type": "operations",
      "operations": 100000
    },
    "measured_operations": 10000000,
    "sample_count": 10000000,
    "clock": "monotonic",
    "instrumentation": [
      "application_timing",
      "perf_stat"
    ]
  }
}
```

Recommended fields:

* warmup type
* warmup amount
* measured operation count
* sample count
* clock/timer source
* instrumentation enabled

Possible future values:

* CUDA events
* Nsight Systems
* Nsight Compute
* NCCL debug output

---

# 13. `metrics`

Recommended structure:

```json
{
  "metrics": {
    "latency": {},
    "throughput": {},
    "counters": {},
    "stages": {},
    "gpu": {},
    "communication": {},
    "correctness": {}
  }
}
```

Not every benchmark needs every subsection.

---

# 14. `metrics.latency`

Canonical time unit:

```text
nanoseconds
```

Example:

```json
{
  "latency": {
    "end_to_end_ns": {
      "count": 10000000,
      "min": 120,
      "p50": 250,
      "p99": 800,
      "p999": 1600,
      "max": 22000,
      "mean": 290.4
    }
  }
}
```

Recommended percentile names:

* `p50`
* `p90`
* `p95`
* `p99`
* `p999`
* `p9999`

Only emit percentiles supported by sufficient sample count and measurement design.

---

## 14.1 Multiple Latency Types

Example:

```json
{
  "latency": {
    "end_to_end_ns": {},
    "producer_push_ns": {},
    "queue_residency_ns": {},
    "consumer_service_ns": {}
  }
}
```

This supports attribution.

---

# 15. `metrics.throughput`

Example:

```json
{
  "throughput": {
    "messages_per_second": 12500000.0,
    "operations_per_second": 12500000.0,
    "bytes_per_second": 800000000.0
  }
}
```

Use only metrics relevant to the workload.

---

# 16. `metrics.counters`

Hardware and OS counters should preserve the raw event identity.

Example:

```json
{
  "counters": {
    "cycles": {
      "value": 1234567890,
      "available": true
    },
    "instructions": {
      "value": 2345678901,
      "available": true
    },
    "cache_misses": {
      "value": null,
      "available": false
    }
  }
}
```

Optional derived section:

```json
{
  "derived": {
    "ipc": 1.9,
    "branch_miss_percent": 0.8
  }
}
```

The analysis system should distinguish:

* measured values
* derived values

---

# 17. `metrics.stages`

Stage-level timing example:

```json
{
  "stages": {
    "cpu_preprocess_ns": {
      "p50": 1000,
      "p99": 1600
    },
    "batch_wait_ns": {
      "p50": 5000,
      "p99": 25000
    },
    "h2d_ns": {
      "p50": 10000,
      "p99": 12000
    },
    "gpu_kernel_ns": {
      "p50": 8000,
      "p99": 9000
    }
  }
}
```

A stage may be represented as:

* summary distribution
* total duration
* both

depending on the workload.

---

# 18. `metrics.gpu`

Example:

```json
{
  "gpu": {
    "batch_size": 1024,
    "stream_count": 2,
    "memory_mode": "pinned",
    "transfer_mode": "async",
    "kernel": {
      "name": "normalize_kernel",
      "grid_size": 80,
      "block_size": 256
    },
    "utilization_percent": null
  }
}
```

Profiler-derived values should identify their source when practical.

---

# 19. `metrics.communication`

Example:

```json
{
  "communication": {
    "collective": "all_reduce",
    "message_bytes": 16777216,
    "world_size": 4,
    "latency_ns": 1250000,
    "algorithm_bandwidth_bytes_per_second": 50000000000,
    "bus_bandwidth_bytes_per_second": 75000000000
  }
}
```

For overlap studies:

```json
{
  "communication": {
    "total_communication_ns": 10000000,
    "exposed_communication_ns": 3000000,
    "hidden_communication_ns": 7000000,
    "overlap_percent": 70.0
  }
}
```

---

# 20. `metrics.correctness`

Performance data should preserve important correctness outcomes.

Example:

```json
{
  "correctness": {
    "messages_produced": 1000000,
    "messages_consumed": 999900,
    "drops": 100,
    "sequence_gaps": 3,
    "duplicates": 0,
    "validation_passed": true
  }
}
```

A faster result that violates correctness must not be treated as an optimization.

---

# 21. `artifacts`

Example:

```json
{
  "artifacts": [
    {
      "type": "perf_stat",
      "path": "perf-stat.txt"
    },
    {
      "type": "flamegraph",
      "path": "profile/flamegraph.svg"
    },
    {
      "type": "nsight_systems",
      "path": "gpu/run.nsys-rep"
    }
  ]
}
```

Possible artifact types:

* console output
* perf stat
* perf data
* FlameGraph
* Nsight Systems
* Nsight Compute
* topology
* NCCL log
* chart
* report

Use relative paths where practical.

---

# 22. `notes`

Example:

```json
{
  "notes": [
    "CPU frequency governor could not be controlled.",
    "The host was shared with other workloads.",
    "One optional PMU counter was unavailable."
  ]
}
```

This section is for run-specific caveats.

It should not replace structured metadata.

---

# 23. Complete CPU Benchmark Example

```json
{
  "schema_version": "1.0",
  "run": {
    "id": "2026-07-06T23-12-04Z-mpsc-bench",
    "timestamp_utc": "2026-07-06T23:12:04Z",
    "trial_index": 1,
    "trial_count": 5
  },
  "benchmark": {
    "name": "mpsc_bench",
    "version": "0.3",
    "category": "concurrency"
  },
  "source": {
    "git_commit": "0123456789abcdef",
    "git_dirty": false
  },
  "build": {
    "type": "Release",
    "compiler": "clang",
    "compiler_version": "18.1.3",
    "language_standard": "C++20",
    "compiler_flags": [
      "-O3",
      "-march=native"
    ]
  },
  "system": {
    "os": {
      "name": "Ubuntu",
      "version": "24.04",
      "kernel": "6.8.0"
    },
    "cpu": {
      "architecture": "x86_64",
      "logical_cpus": 16,
      "physical_cores": 8,
      "smt_enabled": true,
      "cache_line_bytes": 64
    }
  },
  "configuration": {
    "messages": 10000000,
    "queue_capacity": 4096,
    "producer_count": 4,
    "consumer_count": 1,
    "mode": "try_drop",
    "layout": "padded"
  },
  "placement": {
    "cpu_affinity": {
      "producer_cpus": [2, 4, 6, 8],
      "consumer_cpus": [10],
      "policy": "explicit"
    }
  },
  "measurement": {
    "warmup": {
      "type": "operations",
      "operations": 100000
    },
    "measured_operations": 10000000,
    "sample_count": 9999900,
    "clock": "monotonic",
    "instrumentation": [
      "application_timing",
      "perf_stat"
    ]
  },
  "metrics": {
    "latency": {
      "end_to_end_ns": {
        "count": 9999900,
        "min": 120,
        "p50": 250,
        "p99": 800,
        "p999": 1600,
        "max": 22000
      }
    },
    "throughput": {
      "messages_per_second": 12500000.0
    },
    "counters": {
      "cycles": {
        "value": 1234567890,
        "available": true
      },
      "instructions": {
        "value": 2345678901,
        "available": true
      }
    },
    "derived": {
      "ipc": 1.9
    },
    "correctness": {
      "drops": 100,
      "sequence_gaps": 3,
      "duplicates": 0,
      "validation_passed": true
    }
  },
  "artifacts": [
    {
      "type": "perf_stat",
      "path": "perf-stat.txt"
    }
  ],
  "notes": []
}
```

Values above are schema examples only.

They are not real benchmark results.

---

# 24. Complete Heterogeneous Benchmark Example

```json
{
  "schema_version": "1.0",
  "benchmark": {
    "name": "cpu_gpu_pipeline_bench",
    "version": "0.8",
    "category": "heterogeneous"
  },
  "configuration": {
    "messages": 1000000,
    "batch_size": 1024,
    "stream_count": 2
  },
  "placement": {
    "gpu_devices": [0]
  },
  "measurement": {
    "instrumentation": [
      "application_timing",
      "cuda_events",
      "nsight_systems"
    ]
  },
  "metrics": {
    "latency": {
      "end_to_end_ns": {}
    },
    "stages": {
      "cpu_preprocess_ns": {},
      "batch_wait_ns": {},
      "h2d_ns": {},
      "gpu_kernel_ns": {},
      "d2h_ns": {}
    },
    "gpu": {
      "batch_size": 1024,
      "stream_count": 2,
      "memory_mode": "pinned",
      "transfer_mode": "async"
    }
  },
  "artifacts": [
    {
      "type": "nsight_systems",
      "path": "gpu/pipeline.nsys-rep"
    }
  ]
}
```

---

# 25. Result-Schema Implementation Strategy

Do not implement the entire theoretical schema at once.

Recommended sequence:

## v0.4 Initial Implementation

Implement:

* schema version
* run
* benchmark
* source
* build
* basic system CPU/OS metadata
* configuration
* measurement
* latency
* throughput
* perf counters
* artifacts

---

## v0.5 Extension

Add:

* memory policy
* page configuration
* NUMA placement

---

## v0.7 Extension

Add:

* GPU metadata
* batch configuration
* H2D/kernel timing

---

## v0.8 Extension

Add:

* stage timing
* streams
* memory mode
* profiler artifacts

---

## v0.9 Extension

Add:

* topology
* collectives
* communication metrics

---

## v1.0 Extension

Add:

* distributed workload metadata
* scaling metrics
* bottleneck-analysis results

---

# 26. Future Formal JSON Schema

After real v0.4 usage stabilizes the structure, consider adding:

```text
schema/
└── benchmark-result-v1.json
```

The formal schema may validate:

* required top-level fields
* data types
* enum values
* non-negative metrics
* schema versions

Do not create a complex formal schema before the first real JSON emitter proves the design.
