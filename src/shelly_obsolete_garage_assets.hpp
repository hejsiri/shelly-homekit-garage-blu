#pragma once

#include <cstddef>
#include <cstring>

namespace shelly {

// Removed from the firmware image in 53a182d. OTA used to merge these files
// back from the old filesystem, leaving too little room for HomeKit/config
// temporary files on Plus 1. Keep this exact list separate from user data.
static const char *const kObsoleteGarageAssets[] = {
    "garage-car-night.webp",
    "garage-car.webp",
    "garage-door-night.webp",
    "garage-door.webp",
    "garage-house-day.webp",
    "garage-house-night.webp",
    "garage-interior-car-night.webp",
    "garage-interior-car.webp",
    "garage-interior-empty-night.webp",
    "garage-interior-empty.webp",
    "garage-only-preview.webp",
};

inline bool IsObsoleteGarageAsset(const char *name) {
  if (name == nullptr) return false;
  for (const char *asset : kObsoleteGarageAssets) {
    if (std::strcmp(name, asset) == 0) return true;
  }
  return false;
}

template <typename RemoveFile>
size_t RemoveObsoleteGarageAssets(RemoveFile remove_file) {
  size_t removed = 0;
  for (const char *asset : kObsoleteGarageAssets) {
    if (remove_file(asset) == 0) removed++;
  }
  return removed;
}

}  // namespace shelly
