#!/usr/bin/env bash
set -euo pipefail

N="${1:-1000000}"
WARMUP="${2:-100000}"
PORT="${3:-19001}"

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release -DULL_ENABLE_SANITIZERS=OFF
cmake --build build/rel -j
./build/rel/udp_pipeline_bench "$N" "$WARMUP" "$PORT"
