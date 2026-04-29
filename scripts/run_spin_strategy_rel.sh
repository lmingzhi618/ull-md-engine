#! /usr/bin/env bash
set -euo pipefail

STRATEGY="${1:-pure_spin}"
N="${2:-2000000}"
WARMUP="${3:-200000}"

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release -DULL_ENABLE_SANITIZERS=OFF
cmake --build build/rel -j 
./build/rel/spin_strategy_bench "$STRATEGY" "$N" "$WARMUP"
