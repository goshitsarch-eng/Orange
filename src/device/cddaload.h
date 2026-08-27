#ifndef STRAWBERRY_CDDALOAD_H
#define STRAWBERRY_CDDALOAD_H

#include "device/cddatext.h"
#include "tagfetcher/musicbrainzdiscid.h"

#include <string>
#include <vector>

namespace CddaLoad {

// Qt CDDASongLoader: MusicBrainz only when CD-TEXT did not fill every title.
inline bool ShouldLookupMusicBrainz(bool cdtext_complete, const std::string &disc_id, bool have_network, bool have_tagfetcher) {
  return have_tagfetcher && have_network && !cdtext_complete && MusicBrainzDiscId::ShouldLookup(disc_id);
}

inline bool ShouldLookupMusicBrainz(const SongList &songs, bool have_network, bool have_tagfetcher) {
  return ShouldLookupMusicBrainz(CddaText::HasCompleteTitles(songs), MusicBrainzDiscId::DiscIdFromSongs(songs), have_network,
                                 have_tagfetcher);
}

inline bool ShouldEmitTracks(const SongList &songs) { return !songs.empty(); }

inline std::vector<std::string> FallbackPaths(const std::string &requested, const std::vector<std::string> &known) {
  std::vector<std::string> paths;
  if (!requested.empty()) {
    paths.push_back(requested);
  }
  for (const std::string &path : known) {
    if (path.empty() || path == requested) {
      continue;
    }
    paths.push_back(path);
  }
  return paths;
}

}  // namespace CddaLoad

#endif
