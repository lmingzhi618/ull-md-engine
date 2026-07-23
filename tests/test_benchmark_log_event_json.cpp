#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include "ull/bench/benchmark_log_event.h"

#include "benchmark_log_event.cjm.hpp"

#include <nlohmann/json.hpp>

int main() {
    ull::bench::BenchmarkLogEvent event{
        .benchmark_name = "sp_fanout_bench",
        .phase = "complete",
        .timestamp_ns = 123456789,
        .elapsed_ns = 987654321,
        .success = true,
        .note = std::string{"dogfood cjm v0.3.0"},
        .tags = {"dogfood", "json", "benchmark"},
        .counters = {{"messages", 1000}, {"consumers", 2}},
    };

    nlohmann::json j = event;

    assert(j.at("benchmark_name") == "sp_fanout_bench");
    assert(j.at("phase") == "complete");
    assert(j.at("timestamp_ns") == 123456789);
    assert(j.at("elapsed_ns") == 987654321);
    assert(j.at("success") == true);
    assert(j.at("note") == "dogfood cjm v0.3.0");
    assert(j.at("tags").at(0) == "dogfood");
    assert(j.at("counters").at("messages") == 1000);

    const auto decoded = j.get<ull::bench::BenchmarkLogEvent>();

    assert(decoded.benchmark_name == event.benchmark_name);
    assert(decoded.phase == event.phase);
    assert(decoded.timestamp_ns == event.timestamp_ns);
    assert(decoded.elapsed_ns == event.elapsed_ns);
    assert(decoded.success == event.success);
    assert(decoded.note == event.note);
    assert(decoded.tags == event.tags);
    assert(decoded.counters == event.counters);

    std::cout << "test_benchmark_log_event_json PASS\n";
    return 0;
}
