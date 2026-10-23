#!/bin/bash
set -euo pipefail
# mos rejects source paths containing spaces. Build from a user-owned staging
# directory and return artifacts to this checkout.
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${SHELLY_BLU_BUILD_ROOT:-$HOME/.mos/work/shelly-garage-blu}"
mos_bin="${MOS:-$HOME/.mos/bin/mos}"
model="${SHELLY_BLU_MODEL:-Shelly1Gen3}"
case "$model" in
  Shelly1Gen3|ShellyPlus1) ;;
  *) echo "Unsupported BLU model: $model" >&2; exit 1 ;;
esac
mkdir -p "$build_root"
if [[ "$build_root" == *[[:space:]]* || "$build_root" == "$project_root" ]]; then
  echo 'SHELLY_BLU_BUILD_ROOT must be a separate directory without spaces.' >&2
  exit 1
fi
rsync -a --exclude='.git/' --exclude='build*/' --exclude='deps/' --exclude='binlibs/' \
  --exclude='releases/' "$project_root/" "$build_root/"
(cd "$build_root" && make "$model" MOS="$mos_bin" "$@")
mkdir -p "$project_root/build_$model"
cp "$build_root/build_$model/fw.zip" "$project_root/build_$model/fw.zip"
echo "Firmware: $project_root/build_$model/fw.zip"
