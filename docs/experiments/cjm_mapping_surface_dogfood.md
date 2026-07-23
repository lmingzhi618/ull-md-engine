# CJM Mapping Surface Dogfood

## Question

Can CJM v0.3.0 generate working `nlohmann/json` integration for a broader
supported mapping surface in a downstream project?

## Context

This is the second dogfood pass after `BenchmarkLogEvent`.

The first pass validated the basic downstream workflow. This pass validates
more of the documented CJM v0.3.0 mapping surface.

## Model

The model is:

```text
ull::bench::CjmMappingSurfaceEvent
```

## Defined in:

```text
include/ull/bench/cjm_mapping_surface_event.h
```

## Covered mapping features:

| Feature | Field / Type | Result |
| --- | --- | --- |
| enum class | `CjmEventLevel` | passed |
| nested generated struct | `CjmMappingDetail` | passed |
| signed fixed-width integer | `std::int64_t` | passed |
| floating point | `double` | passed |
| optional + omitempty | `std::optional<std::string>` | passed |
| vector | `std::vector<std::int64_t>` | passed |
| unordered string map | `std::unordered_map<std::string, std::uint64_t>` | passed |
| ignored field | `json:"-"` | passed |
| from_json round-trip | full model | passed |

## Validation

Commands:
```bash
cmake -S . -B build/dev -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev --target test_cjm_mapping_surface_json
ctest --test-dir build/dev -R "test_benchmark_log_event_json|test_cjm_mapping_surface_json" --output-on-failure
```

## Result: 

```text
100% tests passed, 0 tests failed out of 2
```

## Observations 

CJM generated:
```text 
build/dev/generated/cjm/cjm_mapping_surface_event.cjm.hpp
```

The generated integration supported:
```cpp
nlohmann::json j = event;
auto decoded = j.get<ull::bench::CjmMappingSurfaceEvent>();
```

The test verified:
- empty_note was omitted when std::nullopt
- internal_debug was not serialized because of json:"-"
- nested struct fields round-tripped
- unordered map fields round-tripped
- enum value round-tripped

## Friction

No new CJM warning or integration issue was observed in this run.
One limitation remains expected: CJM v0.3.0 does not provide enum string mapping
policies, so this test verifies enum round-trip behavior rather than string
enum representation.

## Conclusion

CJM v0.3.0 works in ull-md-engine for a broader practical mapping surface,
not only the initial logging smoke test.
This gives stronger downstream confidence in the v0.3.0 release.

## Next Step

No further CJM expansion is needed inside ull-md-engine right now.
Future CJM usage should be driven by a real benchmark-result or observability
need, not by adding mapping tests indefinitely.
