#!/usr/bin/env bash
set -euo pipefail

PRODUCERS="${1:-1}"
MESSAGES_PER_PRODUCER="${2:-500000}"
WARMUP="${3:-50000}"
CAPACITY="${4:-65536}"

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release -DULL_ENABLE_SANITIZERS=OFF
cmake --build build/rel -j
./build/rel/mpsc_bench "$PRODUCERS" "$MESSAGES_PER_PRODUCER" "$WARMUP" "$CAPACITY"
