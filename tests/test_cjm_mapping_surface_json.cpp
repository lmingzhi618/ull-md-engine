#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include "ull/bench/cjm_mapping_surface_event.h"
#include "cjm_mapping_surface_event.cjm.hpp"

#include <nlohmann/json.hpp>

int main() {
    ull::bench::CjmMappingSurfaceEvent event{
        .level = ull::bench::CjmEventLevel::Warning,
        .detail =
            {
                .signed_delta = -42,
                .ratio = 0.75,
            },
        .empty_note = std::nullopt,
        .samples = {-3, 0, 7},
        .counters = {{"messages", 1000}, {"drops", 2}},
        .internal_debug = "must not be serialized",
    };

    nlohmann::json j = event;
    assert(j.contains("level"));
    assert(j.at("detail").at("signed_delta") == -42);
    assert(j.at("detail").at("ratio") == 0.75);
    assert(!j.contains("empty_note"));
    assert(j.at("samples").at(0) == -3);
    assert(j.at("counters").at("messages") == 1000);
    assert(!j.contains("internal_debug"));

    const auto decoded = j.get<ull::bench::CjmMappingSurfaceEvent>();

    assert(decoded.level == event.level);
    assert(decoded.detail.signed_delta == event.detail.signed_delta);
    assert(decoded.detail.ratio == event.detail.ratio);
    assert(!decoded.empty_note.has_value());
    assert(decoded.samples == event.samples);
    assert(decoded.counters == event.counters);
    assert(decoded.internal_debug.empty());

    std::cout << "test_cjm_mapping_surface_json PASS\n";
    return 0;
}
