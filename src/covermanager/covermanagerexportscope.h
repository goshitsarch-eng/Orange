#ifndef STRAWBERRY_COVERMANAGEREXPORTSCOPE_H
#define STRAWBERRY_COVERMANAGEREXPORTSCOPE_H

#include "covermanager/albumcovermanagerlist.h"
#include "core/song.h"

#include <string>
#include <vector>

namespace CoverManagerExportScope {

inline SongList SongsToExport(const std::vector<AlbumCoverManagerList::Album> &visible) {
  SongList songs;
  for (const auto &album : visible) {
    if (album.has_cover) {
      songs.push_back(album.song);
    }
  }
  return songs;
}

inline const char *NoCoversText() { return "No covers to export."; }

inline const char *FinishedTitle() { return "Export finished"; }

inline std::string FinishedBody(int exported, int skipped) {
  return "Exported " + std::to_string(exported) + " covers (" + std::to_string(skipped) + " skipped).";
}

}  // namespace CoverManagerExportScope

#endif
