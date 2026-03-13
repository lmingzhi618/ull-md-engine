#!/usr/bin/env bash
set -e

MODE=${1:-padded}

cmake -S . -B build/rel -DCMAKE_BUILD_TYPE=Release
cmake --build build/rel -j

./build/rel/false_sharing_microbench $MODE
