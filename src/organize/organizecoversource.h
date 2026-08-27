#ifndef STRAWBERRY_ORGANIZECOVERSOURCE_H
#define STRAWBERRY_ORGANIZECOVERSOURCE_H

#include "core/song.h"
#include "organize/organize.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <cstdio>
#include <string>

namespace OrganizeCoverSource {

inline std::string WriteEmbedded(const TagReader::CoverData &cover, const std::string &cache_path) {
  if (cover.data.empty() || cache_path.empty()) {
    return {};
  }
  FILE *file = std::fopen(cache_path.c_str(), "wb");
  if (!file) {
    return {};
  }
  std::fwrite(cover.data.data(), 1, cover.data.size(), file);
  std::fclose(file);
  return cache_path;
}

inline std::string ForSong(const Song &song, TagReader *tagreader = nullptr, const std::string &cache_path = {}) {
  const std::string sidecar = Organize::CoverPathForSong(song);
  if (!sidecar.empty()) {
    return sidecar;
  }
  if (!tagreader) {
    return {};
  }
  const TagReader::CoverData cover = tagreader->LoadCoverData(FileUtils::PathFromUri(song.url()));
  return WriteEmbedded(cover, cache_path);
}

}  // namespace OrganizeCoverSource

#endif  // STRAWBERRY_ORGANIZECOVERSOURCE_H
