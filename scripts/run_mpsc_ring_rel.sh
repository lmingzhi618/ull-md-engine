#! /usr/bin/env bash
set -euxo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cmake -S "$ROOT_DIR" \
      -B "$ROOT_DIR/build/rel" \
      -DCMAKE_BUILD_TYPE=Release 

cmake --build "$ROOT_DIR/build/rel" -j

"$ROOT_DIR/build/rel/test_mpsc_ring"
