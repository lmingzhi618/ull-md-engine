#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build/dev \
  -DCMAKE_BUILD_TYPE=Debug \
  -DULL_ENABLE_SANITIZERS=OFF

cmake --build build/dev -j

ctest --test-dir build/dev --output-on-failure
