#!/bin/bash
set -euo pipefail
# mos rejects source paths containing spaces. Build from a user-owned staging
# directory and return artifacts to this checkout.
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${SHELLY_BLU_BUILD_ROOT:-$HOME/.mos/work/shelly-garage-blu}"
mos_bin="${MOS:-$HOME/.mos/bin/mos}"
model="${SHELLY_BLU_MODEL:-ShellyPlus1}"
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
  --exclude='releases/' --exclude='design/' --exclude='__pycache__/' "$project_root/" "$build_root/"
# Remove deleted source/assets from the private build staging directory too.
for source_dir in src fs fs_src; do
  rsync -a --delete "$project_root/$source_dir/" "$build_root/$source_dir/"
done
# Staging may receive edits made while a previous build was still running:
# rsync preserves their mtime, which can precede that build's objects. Rebuild
# application objects and the panel while retaining the SDK cache.
rm -f "$build_root/fs/index.html.gz"
app_objects="$build_root/build_$model/objs/esp-idf/mosapp/CMakeFiles/__idf_mosapp.dir$build_root/src"
if [[ -d "$app_objects" ]]; then
  find "$app_objects" -name '*.obj' -delete
fi
(cd "$build_root" && make "$model" MOS="$mos_bin" "$@")
mkdir -p "$project_root/build_$model"
cp "$build_root/build_$model/fw.zip" "$project_root/build_$model/fw.zip"
echo "Firmware: $project_root/build_$model/fw.zip"
