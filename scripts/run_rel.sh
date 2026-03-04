#!/usr/bin/env bash
set -euo pipefail
N="${1:-2000000}"
WARMUP="${2:-200000}"

cmake --preset rel
cmake --build --preset rel -j
./build/rel/ull_bench "$N" "$WARMUP"
