#ifndef STRAWBERRY_COLLECTIONRESCANSONGS_H
#define STRAWBERRY_COLLECTIONRESCANSONGS_H

#include "collection/collectiondirectory.h"
#include "core/song.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace CollectionRescanSongs {

// Qt CollectionWatcher::RescanSongs scans each song's parent directory once.
inline std::string SongPath(const Song &song) { return FileUtils::DirName(FileUtils::PathFromUri(song.url())); }

inline bool ShouldRescanInCollection(const Song &song) { return song.is_collection_song() || song.directory_id() > 0; }

// Qt MainWindow::RescanSongs ReloadItem is only for local files that are not collection items.
inline bool ShouldReloadPlaylistItem(const Song &song) {
  return song.source() == Song::Source::LocalFile && !ShouldRescanInCollection(song);
}

inline bool DirectoryWatched(int directory_id, const std::vector<CollectionDirectory> &directories) {
  if (directory_id < 0) {
    return false;
  }
  for (const CollectionDirectory &directory : directories) {
    if (directory.id == directory_id) {
      return true;
    }
  }
  return false;
}

inline int DirectoryIdForPath(const std::string &path, const std::vector<CollectionDirectory> &directories) {
  int best_id = -1;
  size_t best_len = 0;
  for (const CollectionDirectory &directory : directories) {
    if (directory.path.empty()) {
      continue;
    }
    const bool match = path == directory.path || path.rfind(directory.path + "/", 0) == 0;
    if (match && directory.path.size() >= best_len) {
      best_id = directory.id;
      best_len = directory.path.size();
    }
  }
  return best_id;
}

inline bool ShouldScanPath(const std::string &path, std::vector<std::string> *scanned) {
  if (path.empty() || !scanned) {
    return false;
  }
  if (std::find(scanned->begin(), scanned->end(), path) != scanned->end()) {
    return false;
  }
  scanned->push_back(path);
  return true;
}

using Target = std::pair<int, std::string>;

inline std::vector<Target> Targets(const SongList &songs, const std::vector<CollectionDirectory> &directories) {
  std::vector<Target> targets;
  std::vector<std::string> scanned;
  for (const Song &song : songs) {
    const std::string path = SongPath(song);
    int directory_id = song.directory_id();
    if (!DirectoryWatched(directory_id, directories)) {
      directory_id = DirectoryIdForPath(path, directories);
    }
    if (!DirectoryWatched(directory_id, directories) || !ShouldScanPath(path, &scanned)) {
      continue;
    }
    targets.emplace_back(directory_id, path);
  }
  return targets;
}

inline SongList CollectionSongs(const SongList &songs) {
  SongList collection;
  for (const Song &song : songs) {
    if (ShouldRescanInCollection(song)) {
      collection.push_back(song);
    }
  }
  return collection;
}

inline const char *TaskName() { return "Rescanning songs"; }

// Qt ScanTransaction for RescanSongs does not mark the rest of the collection unavailable.
inline bool MarksMissingUnavailable() { return false; }

}  // namespace CollectionRescanSongs

#endif
