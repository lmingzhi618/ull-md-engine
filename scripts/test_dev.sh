#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build/dev-asan -DCMAKE_BUILD_TYPE=Debug -DULL_ENABLE_SANITIZERS=ON
cmake --build build/dev-asan -j
ctest --test-dir build/dev-asan --output-on-failure
