#ifndef STRAWBERRY_EDITTAGLOADING_H
#define STRAWBERRY_EDITTAGLOADING_H

#include "core/song.h"
#include "tagreader/tagreaderresult.h"
#include "utilities/fileutils.h"

#include <functional>
#include <string>
#include <vector>

namespace EditTagLoading {

// Qt MainWindow::EditFileTags: URL + valid + MPEG only; the dialog reloads tags.
inline Song PlaceholderFromPath(const std::string &path) {
  Song song(Song::Source::LocalFile);
  song.set_valid(true);
  song.set_url(FileUtils::UriFromPath(path));
  song.set_filetype(Song::FileType::MPEG);
  return song;
}

inline SongList PlaceholdersFromPaths(const std::vector<std::string> &paths) {
  SongList songs;
  for (const std::string &path : paths) {
    if (path.empty() || FileUtils::IsDirectory(path)) {
      continue;
    }
    songs.push_back(PlaceholderFromPath(path));
  }
  return songs;
}

inline bool OpensDialog(const SongList &songs) { return !songs.empty(); }

inline bool KeepRead(const TagReaderResult &result, const Song &song) { return result.success() && song.is_valid(); }

using ReadBlocking = std::function<TagReaderResult(const std::string &, Song *)>;

// Qt EditTagDialog::LoadData: reread editable local files and keep only successful reads.
inline SongList LoadData(const SongList &songs, const ReadBlocking &read) {
  SongList loaded;
  if (!read) {
    return loaded;
  }
  for (const Song &song : songs) {
    if (!song.IsEditable()) {
      continue;
    }
    Song copy = song;
    const TagReaderResult result = read(FileUtils::PathFromUri(copy.url()), &copy);
    if (KeepRead(result, copy)) {
      copy.MergeUserSetData(song, false, false);
      loaded.push_back(copy);
    }
  }
  return loaded;
}

}  // namespace EditTagLoading

#endif
