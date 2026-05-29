#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-push}"
PRODUCERS="${2:-1}"
MESSAGES_PER_PRODUCER="${3:-500000}"
WARMUP="${4:-50000}"
CAPACITY="${5:-65536}"

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release -DULL_ENABLE_SANITIZERS=OFF
cmake --build build/rel -j
./build/rel/mpsc_bench "$MODE" "$PRODUCERS" "$MESSAGES_PER_PRODUCER" "$WARMUP" "$CAPACITY"
