#ifndef STRAWBERRY_MPRIS2PLAYLISTS_H
#define STRAWBERRY_MPRIS2PLAYLISTS_H

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

namespace Mpris2Playlists {

inline const char *kInterface = "org.mpris.MediaPlayer2.Playlists";
inline const char *kPathPrefix = "/org/strawberrymusicplayer/strawberry/PlaylistId/";
inline const char *kOrderUserDefined = "UserDefined";
inline const char *kOrderAlphabetical = "Alphabetical";
inline const char *kPlaylistChangedSignal = "PlaylistChanged";
inline const char *kPlaylistCountProperty = "PlaylistCount";
inline const char *kActivePlaylistProperty = "ActivePlaylist";
inline const char *kInactivePlaylistPath = "/";

struct Entry {
  std::string id;
  std::string name;
  std::string icon;
};

inline std::string ObjectPath(int id) { return std::string(kPathPrefix) + std::to_string(id); }

inline Entry ChangedEntry(int id, const std::string &name, const std::string &icon = {}) { return {ObjectPath(id), name, icon}; }

inline bool ShouldNotifyCountOnCollectionChange() { return true; }

inline bool ShouldNotifyActiveOnCurrentChange() { return true; }

inline int IdFromPath(const std::string &path) {
  const std::string prefix = kPathPrefix;
  if (path.rfind(prefix, 0) != 0) {
    return -1;
  }
  return std::atoi(path.c_str() + prefix.size());
}

inline bool IsAlphabetical(const std::string &order) { return order == kOrderAlphabetical; }

inline std::vector<Entry> SortAndSlice(std::vector<Entry> entries, const std::string &order, bool reverse, unsigned index,
                                       unsigned max_count) {
  if (IsAlphabetical(order)) {
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) { return a.name < b.name; });
  } else {
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) { return a.id < b.id; });
  }
  if (reverse) {
    std::reverse(entries.begin(), entries.end());
  }
  if (index >= entries.size()) {
    return {};
  }
  const size_t start = static_cast<size_t>(index);
  const size_t count = max_count == 0 ? 0 : std::min(static_cast<size_t>(max_count), entries.size() - start);
  return std::vector<Entry>(entries.begin() + static_cast<std::ptrdiff_t>(start),
                            entries.begin() + static_cast<std::ptrdiff_t>(start + count));
}

inline std::vector<const char *> Orderings() { return {kOrderUserDefined, kOrderAlphabetical}; }

}  // namespace Mpris2Playlists

#endif  // STRAWBERRY_MPRIS2PLAYLISTS_H
