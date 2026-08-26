#ifndef STRAWBERRY_CDDAHELPERS_H
#define STRAWBERRY_CDDAHELPERS_H

#include "config.h"

#include <cctype>
#include <cstring>
#include <string>

#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#endif

namespace CddaHelpers {

inline bool EndsWithIgnoreCase(const std::string &path, const char *suffix) {
  if (!suffix) {
    return false;
  }
  const size_t suffix_len = std::strlen(suffix);
  if (path.size() < suffix_len) {
    return false;
  }
  for (size_t i = 0; i < suffix_len; ++i) {
    const unsigned char a = static_cast<unsigned char>(path[path.size() - suffix_len + i]);
    const unsigned char b = static_cast<unsigned char>(suffix[i]);
    if (std::tolower(a) != std::tolower(b)) {
      return false;
    }
  }
  return true;
}

inline bool ShouldSkipDevice(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  return EndsWithIgnoreCase(path, ".nrg") || EndsWithIgnoreCase(path, ".iso") || EndsWithIgnoreCase(path, ".cue") ||
         EndsWithIgnoreCase(path, ".bin") || EndsWithIgnoreCase(path, ".img");
}

inline bool IsValidTrackRange(int first, int last) { return first > 0 && last >= first; }

inline bool ShouldAddGenericCdda(int listed_count) { return listed_count <= 0; }

inline void EnsureInit() {
#ifdef HAVE_AUDIOCD
  static bool initialized = false;
  if (!initialized) {
    cdio_init();
    initialized = true;
  }
#endif
}

}  // namespace CddaHelpers

#endif  // STRAWBERRY_CDDAHELPERS_H
