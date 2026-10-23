#!/bin/bash
set -euo pipefail
project_root="$(cd "$(dirname "$0")/.." && pwd)"
mbedtls_source="${SHELLY_MBEDTLS_SOURCE:-$HOME/.mos/work/shelly-garage-blu/deps/modules/mbedtls}"
crypto_build="${SHELLY_MBEDTLS_HOST_BUILD:-$HOME/.mos/work/mbedtls-host}"
cmake_tool="${CMAKE:-$HOME/.mos/python/cmake/data/bin/cmake}"
if [[ ! -f "$mbedtls_source/include/mbedtls/ccm.h" ]]; then
  echo 'Build the Gen3 firmware first, or set SHELLY_MBEDTLS_SOURCE.' >&2
  exit 1
fi
if [[ ! -f "$crypto_build/library/libmbedcrypto.a" ]]; then
  "$cmake_tool" -S "$mbedtls_source" -B "$crypto_build" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF
  "$cmake_tool" --build "$crypto_build" --target mbedcrypto -j4
fi
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/shelly-blu-tests.XXXXXX")"
trap 'rm -rf "$test_dir"' EXIT
cd "$project_root"
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src tests/blu_bthome_test.cpp -o "$test_dir/decoder"
"$test_dir/decoder"
clang++ -std=c++14 -DSHELLY_GARAGE_BLE -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src -I tests/stubs -I "$mbedtls_source/include" tests/blu_secure_test.cpp \
  src/shelly_blu_keys.cpp "$crypto_build/library/libmbedcrypto.a" -o "$test_dir/secure"
"$test_dir/secure" "$test_dir/nvs.bin"
"$test_dir/secure" "$test_dir/nvs.bin" restart
