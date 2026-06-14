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
  src/perf/thread_affinity.cpp
  src/net/udp_receiver.cpp
# src/proto/simple_binary.cpp
)

target_include_directories(ull PUBLIC include)

#add_executable(ull_replay src/app/replay_main.cpp)
#target_link_libraries(ull_replay PRIVATE ull)


add_executable(ull_bench src/app/bench_main.cpp)
target_link_libraries(ull_bench PRIVATE ull)

add_executable(udp_pipeline_bench src/app/udp_pipeline_main.cpp)
target_link_libraries(udp_pipeline_bench PRIVATE ull)

add_executable(spin_strategy_bench src/app/spin_strategy_bench_main.cpp)
target_link_libraries(spin_strategy_bench PRIVATE ull)


add_executable(false_sharing_bench src/app/false_sharing_bench_main.cpp)
target_link_libraries(false_sharing_bench PRIVATE ull)

add_executable(false_sharing_microbench src/app/false_sharing_microbench_main.cpp)
target_link_libraries(false_sharing_microbench PRIVATE ull)

find_library(GPERFTOOLS_PROFILER_LIB profiler)
add_executable(busy_vs_blocking_bench src/app/busy_vs_blocking_bench_main.cpp)
target_link_libraries(busy_vs_blocking_bench PRIVATE ull ${GPERFTOOLS_PROFILER_LIB})
target_compile_options(busy_vs_blocking_bench PRIVATE -O2 -g -fno-omit-frame-pointer)

add_executable(mpsc_bench src/app/mpsc_bench_main.cpp)
target_link_libraries(mpsc_bench PRIVATE ull)

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

add_executable(test_simple_binary tests/test_simple_binary.cpp)
target_link_libraries(test_simple_binary PRIVATE ull)
add_test(NAME test_simple_binary COMMAND test_simple_binary)

add_executable(test_udp_receiver tests/test_udp_receiver.cpp)
target_link_libraries(test_udp_receiver PRIVATE ull)
add_test(NAME test_udp_receiver COMMAND test_udp_receiver)

add_executable(test_spsc_ring_unpadded tests/test_spsc_ring_unpadded.cpp)
target_link_libraries(test_spsc_ring_unpadded PRIVATE ull)
add_test(NAME test_spsc_ring_unpadded COMMAND test_spsc_ring_unpadded)

add_executable(test_blocking_queue tests/test_blocking_queue.cpp)
target_link_libraries(test_blocking_queue PRIVATE ull)
add_test(NAME test_blocking_queue COMMAND test_blocking_queue)

add_executable(test_mpsc_ring tests/test_mpsc_ring.cpp)
target_link_libraries(test_mpsc_ring PRIVATE ull)
add_test(NAME test_mpsc_ring COMMAND test_mpsc_ring)


add_executable(test_mpsc_ring_padded tests/test_mpsc_ring_padded.cpp)
target_link_libraries(test_mpsc_ring_padded PRIVATE ull)
add_test(NAME test_mpsc_ring_padded COMMAND test_mpsc_ring_padded)

add_executable(test_mpsc_ring_cell_padded tests/test_mpsc_ring_cell_padded.cpp)
target_link_libraries(test_mpsc_ring_cell_padded PRIVATE ull)
add_test(NAME test_mpsc_ring_cell_padded COMMAND test_mpsc_ring_cell_padded)

add_executable(test_sequence tests/test_sequence.cpp) 
target_link_libraries(test_sequence PRIVATE ull)
add_test(NAME test_sequence COMMAND test_sequence)

add_executable(test_single_producer_sequencer tests/test_single_producer_sequencer.cpp)
target_link_libraries(test_single_producer_sequencer PRIVATE ull)
add_test(NAME test_single_producer_sequencer COMMAND test_single_producer_sequencer)


add_executable(test_single_producer_sequencer_ring tests/test_single_producer_sequencer_ring.cpp)
target_link_libraries(test_single_producer_sequencer_ring PRIVATE ull)
add_test(NAME test_single_producer_sequencer_ring COMMAND test_single_producer_sequencer_ring)

