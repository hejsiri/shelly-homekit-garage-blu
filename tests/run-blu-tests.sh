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
python3 tests/wifi_power_save_test.py
node tests/wifi_power_save_ui_test.js
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src tests/blu_bthome_test.cpp -o "$test_dir/decoder"
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/blu_xiaomi_test.cpp -o "$test_dir/xiaomi"
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/blu_qingping_test.cpp -o "$test_dir/qingping"
"$test_dir/qingping"
"$test_dir/xiaomi"
"$test_dir/decoder"
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/blu_button_test.cpp -o "$test_dir/button"
"$test_dir/button"
clang++ -std=c++14 -DSHELLY_GARAGE_BLE -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src -I tests/stubs -I "$mbedtls_source/include" tests/blu_secure_test.cpp \
  src/shelly_blu_keys.cpp "$crypto_build/library/libmbedcrypto.a" -o "$test_dir/secure"
"$test_dir/secure" "$test_dir/nvs.bin"
"$test_dir/secure" "$test_dir/nvs.bin" restart
clang++ -std=c++14 -DSHELLY_GARAGE_BLE -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I src -I tests/stubs -I "$mbedtls_source/include" tests/blu_battery_cache_test.cpp \
  "$crypto_build/library/libmbedcrypto.a" -o "$test_dir/battery-cache"
"$test_dir/battery-cache"
python3 tests/blu_xiaomi_gatt_test.py
python3 tests/blu_xiaomi_garage_test.py
python3 tests/blu_status_json_test.py
python3 tests/blu_hap_topology_test.py
python3 tests/blu_hap_layout_migration_test.py
python3 tests/blu_hap_database_test.py
python3 tests/garage_fs_cleanup_test.py
python3 tests/homekit_kvs_persistence_test.py
python3 tests/auth_file_persistence_test.py
python3 tests/ota_fs_budget_test.py
python3 tests/blu_button_hap_test.py
python3 tests/blu_key_read_test.py
python3 tests/blu_bond_reset_test.py
clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/blu_bond_cleanup_test.cpp -o "$test_dir/bond-cleanup"
"$test_dir/bond-cleanup"
python3 tests/blu_unpair_test.py
python3 tests/blu_bindkey_rpc_test.py
python3 tests/blu_security_start_test.py

python3 tests/garage_stop_behavior_test.py

python3 tests/garage_endpoint_recovery_test.py
python3 tests/blu_names_config_test.py
node tests/garage_dashboard_visibility_test.js
