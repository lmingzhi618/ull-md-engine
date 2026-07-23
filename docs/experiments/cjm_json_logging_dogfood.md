# CJM JSON Logging Dogfood

## Question

Can `ull-md-engine` consume CJM v0.3.0 as a normal downstream CMake project
and use generated `nlohmann/json` integration for a small logging model?

## Context

CJM is a build-time metadata compiler for Modern C++.

This experiment validates CJM through its public workflow:

```text
FetchContent
-> FetchContent_MakeAvailable(cxx_json_codegen)
-> cjm_generate(...)
-> generated *.cjm.hpp
-> normal C++ test target
```

This is a dogfood experiment, not a logging framework redesign.

## Model

The test model is:

```text
ull::bench::BenchmarkLogEvent
```

Defined in:

```text
include/ull/bench/benchmark_log_event.h
```

Fields used:

| Field | Type | CJM v0.3.0 support |
| --- | --- | --- |
| `benchmark_name` | `std::string` | supported |
| `phase` | `std::string` | supported |
| `timestamp_ns` | `std::uint64_t` | supported |
| `elapsed_ns` | `std::uint64_t` | supported |
| `success` | `bool` | supported |
| `note` | `std::optional<std::string>` | supported |
| `tags` | `std::vector<std::string>` | supported |
| `counters` | `std::map<std::string, std::uint64_t>` | supported |

## Implementation

CJM is integrated through CMake:

```cmake
FetchContent_Declare(
  cxx_json_codegen
  GIT_REPOSITORY https://github.com/cjm-labs/cxx-json-codegen.git
  GIT_TAG v0.3.0
)
FetchContent_MakeAvailable(cxx_json_codegen)

cjm_generate(
  TARGET test_benchmark_log_event_json
  HEADERS include/ull/bench/benchmark_log_event.h
)
```

The generated header is:

```text
build/dev/generated/cjm/benchmark_log_event.cjm.hpp
```

The runtime test includes the model first, then the generated integration:

```cpp
#include "ull/bench/benchmark_log_event.h"
#include "benchmark_log_event.cjm.hpp"
```

## Validation

Commands:

```bash
cmake -S . -B build/dev -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev --target test_benchmark_log_event_json
./build/dev/test_benchmark_log_event_json
```

Result:

```text
test_benchmark_log_event_json PASS
```

## Observations

CJM successfully generated downstream JSON integration for:

- scalar fields
- `std::optional<std::string>`
- `std::vector<std::string>`
- `std::map<std::string, std::uint64_t>`

The generated integration supports the normal `nlohmann/json` workflow:

```cpp
nlohmann::json j = event;
auto decoded = j.get<ull::bench::BenchmarkLogEvent>();
```

## Friction

CJM v0.3.0 emitted one warning when built as a downstream dependency:

```text
warning: comparison of integers of different signs: 'int' and 'std::size_t'
_deps/cxx_json_codegen-src/src/frontends/cxx/semantic/analysis.cpp:82
```

This did not block the downstream build.

One usability note: the generated file extension is `.cjm.hpp`. It is easy to
accidentally include `.cjm.cpp` on the first attempt.

## Conclusion

CJM v0.3.0 works as a downstream CMake dependency in `ull-md-engine` for a small
JSON logging model.

The experiment validates the public workflow without modifying CJM internals or
introducing a broader logging framework.

## Next Step

Report the downstream result and warning back to the CJM project.

Do not expand logging architecture in `ull-md-engine` until there is a real
benchmark-result or runtime-observability use case.
