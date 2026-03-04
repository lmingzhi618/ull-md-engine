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

#enable_testing()
#add_executable(ull_tests tests/basic_tests.cpp)
#target_link_libraries(ull_tests PRIVATE ull)
#add_test(NAME ull_basic_tests COMMAND ull_tests)
