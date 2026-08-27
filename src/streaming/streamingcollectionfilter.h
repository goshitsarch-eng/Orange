#ifndef STRAWBERRY_STREAMINGCOLLECTIONFILTER_H
#define STRAWBERRY_STREAMINGCOLLECTIONFILTER_H

#include "collection/collectionfilteroptions.h"
#include "core/song.h"
#include "utilities/strutils.h"

#include <map>
#include <string>

namespace StreamingCollectionFilter {

// Qt CollectionQuery Untagged: artist, album, or title is empty.
inline bool IsUntagged(const Song &song) { return song.artist().empty() || song.album().empty() || song.title().empty(); }

// Qt CollectionQuery duplicated_songs join on artist/album/title.
inline std::string DuplicateKey(const Song &song) { return song.artist() + '\n' + song.album() + '\n' + song.title(); }

inline bool TextMatches(const Song &song, const std::string &text) {
  if (text.empty()) {
    return true;
  }
  return StrUtils::ContainsInsensitive(song.PrettyTitleWithArtist(), text) || StrUtils::ContainsInsensitive(song.album(), text);
}

inline bool TextSearchEnabled(const CollectionFilterOptions &options) { return options.TextSearchEnabled(); }

inline SongList Apply(const SongList &songs, const CollectionFilterOptions &options, const std::string &text) {
  std::map<std::string, int> counts;
  if (options.filter_mode() == CollectionFilterOptions::FilterMode::Duplicates) {
    for (const Song &song : songs) {
      ++counts[DuplicateKey(song)];
    }
  }
  SongList visible;
  for (const Song &song : songs) {
    if (!options.Matches(song)) {
      continue;
    }
    if (options.filter_mode() == CollectionFilterOptions::FilterMode::Untagged && !IsUntagged(song)) {
      continue;
    }
    if (options.filter_mode() == CollectionFilterOptions::FilterMode::Duplicates && counts[DuplicateKey(song)] < 2) {
      continue;
    }
    if (TextSearchEnabled(options) && !TextMatches(song, text)) {
      continue;
    }
    visible.push_back(song);
  }
  return visible;
}

}  // namespace StreamingCollectionFilter

#endif
