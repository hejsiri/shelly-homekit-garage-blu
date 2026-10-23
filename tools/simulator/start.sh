#!/bin/bash
set -euo pipefail
sim_dir="$(cd "$(dirname "$0")" && pwd)"
clang++ -std=c++14 -dynamiclib -O2 -I "$sim_dir/../../src" "$sim_dir/bridge.cpp" -o "$sim_dir/blu-sim.dylib"
exec python3 "$sim_dir/server.py"
