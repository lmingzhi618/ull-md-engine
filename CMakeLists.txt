cmake_minimum_required(VERSION 3.20)
project(ull_md_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ULL_ENABLE_SANITIZERS "Enable ASAN/UBSAN (debug)" OFF)

if (ULL_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Werror=return-type
)

include_directories(include)

add_library(ull
  src/perf/latency_hist.cpp
  src/perf/ticks.cpp
# src/net/udp_receiver.cpp
# src/protocol/simple_binary.cpp
)

target_include_directories(ull PUBLIC include)

#add_executable(ull_replay src/app/replay_main.cpp)
#target_link_libraries(ull_replay PRIVATE ull)

add_executable(ull_bench src/app/bench_main.cpp)
target_link_libraries(ull_bench PRIVATE ull)

enable_testing()
add_executable(test_ticks tests/test_ticks.cpp)
target_link_libraries(test_ticks PRIVATE ull)
add_test(NAME test_ticks COMMAND test_ticks)

add_executable(test_spsc_ring tests/test_spsc_ring.cpp)
target_link_libraries(test_spsc_ring PRIVATE ull)
add_test(NAME test_spsc_ring COMMAND test_spsc_ring)

add_executable(test_latency_hist tests/test_latency_hist.cpp)
target_link_libraries(test_latency_hist PRIVATE ull)
add_test(NAME test_latency_hist COMMAND test_latency_hist)
