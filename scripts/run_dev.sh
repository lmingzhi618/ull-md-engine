#!/usr/bin/env bash
set -euo pipefail
cmake --preset dev-asan
cmake --build --preset dev-asan -j
./build/dev-asan/ull_bench "${1:-200000}" "${2:-20000}"
