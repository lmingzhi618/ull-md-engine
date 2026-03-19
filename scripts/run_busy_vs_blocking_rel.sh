#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-busy}"
N="${2:-2000000}"
WARMUP="${3:-200000}"

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release -DULL_ENABLE_SANITIZERS=OFF
cmake --build build/rel -j
./build/rel/busy_vs_blocking_bench "$MODE" "$N" "$WARMUP"
