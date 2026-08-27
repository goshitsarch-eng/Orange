#ifndef STRAWBERRY_EDITTAGSAVE_H
#define STRAWBERRY_EDITTAGSAVE_H

#include "core/song.h"

#include <string>
#include <utility>
#include <vector>

namespace EditTagSave {

// Qt MainWindow::EditTagDialogAccepted: stream-service metadata is applied in place.
inline bool ShouldApplyStreamMetadata(const Song &song) { return song.is_stream_service(); }

// Local files are reread so the playlist picks up the tag writer's normalization.
inline bool ShouldReloadFromDisk(const Song &song) { return !ShouldApplyStreamMetadata(song); }

inline bool HasPlaylistSource(const std::vector<int> &rows) { return !rows.empty(); }

// Qt EditTracks only keeps IsEditable songs and their playlist items.
inline std::vector<int> RowsForValidSongs(const SongList &songs, const std::vector<int> &rows) {
  std::vector<int> out;
  const size_t n = std::min(songs.size(), rows.size());
  for (size_t i = 0; i < n; ++i) {
    if (songs[i].IsEditable()) {
      out.push_back(rows[i]);
    }
  }
  return out;
}

// LoadData may drop failed reads; keep the row that shares the loaded song URL.
inline std::vector<int> RowsForLoaded(const SongList &original, const std::vector<int> &rows, const SongList &loaded) {
  std::vector<int> out;
  if (loaded.empty()) {
    return out;
  }
  std::vector<std::pair<std::string, int>> url_row;
  const size_t n = std::min(original.size(), rows.size());
  for (size_t i = 0; i < n; ++i) {
    if (!original[i].url().empty()) {
      url_row.emplace_back(original[i].url(), rows[i]);
    }
  }
  for (const Song &song : loaded) {
    int row = -1;
    for (const auto &entry : url_row) {
      if (entry.first == song.url()) {
        row = entry.second;
        break;
      }
    }
    out.push_back(row);
  }
  return out;
}

// Prefer the stored row when it still points at the same URL; otherwise search.
inline int ResolveRow(const SongList &playlist_songs, int stored_row, const std::string &url) {
  if (url.empty()) {
    return -1;
  }
  if (stored_row >= 0 && stored_row < static_cast<int>(playlist_songs.size()) && playlist_songs[static_cast<size_t>(stored_row)].url() == url) {
    return stored_row;
  }
  for (int i = 0; i < static_cast<int>(playlist_songs.size()); ++i) {
    if (playlist_songs[static_cast<size_t>(i)].url() == url) {
      return i;
    }
  }
  return -1;
}

inline std::vector<int> ResolvedRows(const SongList &playlist_songs, const std::vector<int> &rows, const SongList &songs) {
  std::vector<int> out;
  const size_t n = std::min(rows.size(), songs.size());
  for (size_t i = 0; i < n; ++i) {
    out.push_back(ResolveRow(playlist_songs, rows[i], songs[i].url()));
  }
  return out;
}

inline std::vector<int> RowsToReload(const std::vector<int> &rows, const SongList &songs) {
  std::vector<int> out;
  const size_t n = std::min(rows.size(), songs.size());
  for (size_t i = 0; i < n; ++i) {
    if (rows[i] >= 0 && ShouldReloadFromDisk(songs[i])) {
      out.push_back(rows[i]);
    }
  }
  return out;
}

inline std::vector<std::pair<int, Song>> StreamMetadataUpdates(const std::vector<int> &rows, const SongList &songs) {
  std::vector<std::pair<int, Song>> out;
  const size_t n = std::min(rows.size(), songs.size());
  for (size_t i = 0; i < n; ++i) {
    if (rows[i] >= 0 && ShouldApplyStreamMetadata(songs[i])) {
      out.emplace_back(rows[i], songs[i]);
    }
  }
  return out;
}

// Qt skips WriteFile for streams; metadata is applied to the playlist item instead.
inline bool ShouldWriteFile(const Song &song) { return !song.is_stream(); }

inline bool ShouldPersist(const std::vector<int> &rows) { return HasPlaylistSource(rows); }

// Playlist Edit tag: keep only in-range rows so songs and rows stay aligned.
inline void CollectPlaylistSelection(const SongList &playlist_songs, const std::vector<int> &rows, SongList *songs, std::vector<int> *aligned) {
  if (!songs || !aligned) {
    return;
  }
  songs->clear();
  aligned->clear();
  for (int row : rows) {
    if (row < 0 || row >= static_cast<int>(playlist_songs.size())) {
      continue;
    }
    songs->push_back(playlist_songs[static_cast<size_t>(row)]);
    aligned->push_back(row);
  }
}

}  // namespace EditTagSave

#endif
