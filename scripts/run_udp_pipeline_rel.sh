#!/usr/bin/env bash
set -euo pipefail

N="${1:-2000000}"
WARMUP="${2:-200000}"
PORT="${3:-19001}"

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Debug -DULL_ENABLE_SANITIZERS=ON
cmake --build build/rel -j
./build/rel/udp_pipeline_bench "$N" "$WARMUP" "$PORT"
