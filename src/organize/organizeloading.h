#ifndef STRAWBERRY_ORGANIZELOADING_H
#define STRAWBERRY_ORGANIZELOADING_H

#include "core/song.h"
#include "tagreader/tagreaderresult.h"
#include "utilities/fileutils.h"

#include <functional>
#include <string>
#include <vector>

namespace OrganizeLoading {

inline const char *LoadingText() { return "Loading..."; }

inline const char *PreviewChild() { return "preview"; }

inline const char *LoadingChild() { return "loading"; }

inline const char *VisibleChild(bool loading) { return loading ? LoadingChild() : PreviewChild(); }

inline bool RunEnabled(bool loading, bool can_run) { return !loading && can_run; }

inline bool ShouldLoadFilenames(const SongList &songs, const std::vector<std::string> &filenames) {
  return songs.empty() && !filenames.empty();
}

inline bool UsesPlaylistFallback(bool loading, bool from_filenames) { return !loading && !from_filenames; }

inline std::string StatusText(bool loading, const SongList &songs, bool from_filenames) {
  if (loading) {
    return LoadingText();
  }
  if (songs.empty() && !from_filenames) {
    return "Uses the current playlist as the source.";
  }
  return std::to_string(songs.size()) + " selected song(s).";
}

// Qt CopyFilesToCollection / MoveFilesToCollection passes every selected path to SetFilenames.
// Directories are expanded while tags are read, not added as collection roots first.
inline std::vector<std::string> FileViewFilenames(const std::vector<std::string> &paths) { return paths; }

// Qt OrganizeDialog::LoadSongsBlocking walks QDir::Dirs|Files|NoDotAndDotDot|Readable (hidden files included).
inline std::vector<std::string> ExpandFilenames(const std::vector<std::string> &paths) {
  std::vector<std::string> files;
  std::vector<std::string> queue = paths;
  while (!queue.empty()) {
    const std::string path = queue.front();
    queue.erase(queue.begin());
    if (path.empty()) {
      continue;
    }
    if (FileUtils::IsDirectory(path)) {
      const std::vector<std::string> entries = FileUtils::ListDirectory(path);
      queue.insert(queue.end(), entries.begin(), entries.end());
      continue;
    }
    files.push_back(path);
  }
  return files;
}

using ReadBlocking = std::function<TagReaderResult(const std::string &, Song *)>;

// Qt LoadSongsBlocking keeps a song only when ReadFileBlocking succeeds and the song is valid.
inline SongList SongsFromFilenames(const std::vector<std::string> &paths, const ReadBlocking &read) {
  SongList songs;
  if (!read) {
    return songs;
  }
  for (const std::string &path : ExpandFilenames(paths)) {
    Song song;
    const TagReaderResult result = read(path, &song);
    if (result.success() && song.is_valid()) {
      songs.push_back(song);
    }
  }
  return songs;
}

}  // namespace OrganizeLoading

#endif
