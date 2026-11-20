#!/bin/bash
set -euo pipefail
# mos rejects source paths containing spaces. Build from a user-owned staging
# directory and return artifacts to this checkout.
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${SHELLY_BLU_BUILD_ROOT:-$HOME/.mos/work/shelly-garage-blu}"
mos_bin="${MOS:-$HOME/.mos/bin/mos}"
model="${SHELLY_BLU_MODEL:-ShellyPlus1}"
case "$model" in
  Shelly1Gen3) platform=esp32c3 ;;
  ShellyPlus1) platform=esp32 ;;
  *) echo "Unsupported BLU model: $model" >&2; exit 1 ;;
esac
mkdir -p "$build_root"
if [[ "$build_root" == *[[:space:]]* || "$build_root" == "$project_root" ]]; then
  echo 'SHELLY_BLU_BUILD_ROOT must be a separate directory without spaces.' >&2
  exit 1
fi
rsync -a --exclude='.git/' --exclude='build*/' --exclude='deps/' --exclude='binlibs/' \
  --exclude='releases/' --exclude='design/' --exclude='__pycache__/' "$project_root/" "$build_root/"
# Remove deleted source/assets from the private build staging directory too.
for source_dir in src fs fs_src; do
  rsync -a --delete "$project_root/$source_dir/" "$build_root/$source_dir/"
done
# Fetch/prepare dependencies even on a pristine checkout, then apply the
# reviewed persistence fix before any firmware compilation. A changed upstream
# implementation fails the build instead of silently dropping the fix.
(cd "$build_root" && "$mos_bin" build --platform="$platform" \
  --build-var="MODEL=$model" --build-dir="./build_$model" \
  --binary-libs-dir=./binlibs --local --build-dry-run \
  --repo https://github.com/markirb/mongoose-os)
python3 "$project_root/tools/patch-homekit-kvs.py" \
  "$build_root/deps/homekit-adk/src/PAL/HAPPlatformKeyValueStore.cpp"
# Staging may receive edits made while a previous build was still running:
# rsync preserves their mtime, which can precede that build's objects. Rebuild
# application objects and the panel while retaining the SDK cache.
rm -f "$build_root/fs/index.html.gz"
app_objects="$build_root/build_$model/objs/esp-idf/mosapp/CMakeFiles/__idf_mosapp.dir$build_root/src"
if [[ -d "$app_objects" ]]; then
  find "$app_objects" -name '*.obj' -delete
fi
(cd "$build_root" && make "$model" MOS="$mos_bin" "$@")
python3 "$project_root/tools/ota/check_fs_budget.py" \
  "$build_root/build_$model/fw.zip" \
  --lfs-source "$build_root/deps/vfs-fs-lfs/littlefs"
output_dir="${SHELLY_BLU_OUTPUT_DIR:-$project_root/build_$model}"
mkdir -p "$output_dir"
cp "$build_root/build_$model/fw.zip" "$output_dir/fw.zip"
echo "Firmware: $output_dir/fw.zip"
