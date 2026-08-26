#include "playlist/playlist.h"

#include "playlist/playlistbehaviour.h"
#include "playlist/playlistfilter.h"
#include "playlist/playlistshuffle.h"
#include "playlist/playlistdelegates.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <numeric>
#include <random>

Playlist::Playlist() = default;

void Playlist::set_current_row(int row) {
  if (songs_.empty()) {
    current_row_ = -1;
    current_virtual_index_ = -1;
    CurrentChanged.Emit(current_row_);
    return;
  }
  current_row_ = std::clamp(row, 0, static_cast<int>(songs_.size()) - 1);
  SyncVirtualIndex();
  CurrentChanged.Emit(current_row_);
}

Song Playlist::current_song() const { return song(current_row_); }

Song Playlist::song(int row) const {
  if (row < 0 || row >= static_cast<int>(songs_.size())) {
    return Song();
  }
  return songs_[static_cast<size_t>(row)];
}

Song Playlist::PeekNextSong() const { return song(NextIndex()); }

void Playlist::PushUndo() {
  undo_.push_back({songs_, current_row_});
  redo_.clear();
  if (undo_.size() > 50) {
    undo_.erase(undo_.begin());
  }
}

void Playlist::Undo() {
  if (undo_.empty()) {
    return;
  }
  redo_.push_back({songs_, current_row_});
  songs_ = undo_.back().songs;
  current_row_ = undo_.back().current_row;
  undo_.pop_back();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Redo() {
  if (redo_.empty()) {
    return;
  }
  undo_.push_back({songs_, current_row_});
  songs_ = redo_.back().songs;
  current_row_ = redo_.back().current_row;
  redo_.pop_back();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::ReplaceSongs(const SongList &songs) {
  PushUndo();
  songs_ = songs;
  if (songs_.empty()) {
    current_row_ = -1;
  } else if (current_row_ < 0 || current_row_ >= static_cast<int>(songs_.size())) {
    current_row_ = 0;
  }
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::InsertSongs(int row, const SongList &songs) {
  if (songs.empty()) {
    return;
  }
  PushUndo();
  if (row < 0 || row > static_cast<int>(songs_.size())) {
    row = static_cast<int>(songs_.size());
  }
  songs_.insert(songs_.begin() + row, songs.begin(), songs.end());
  if (current_row_ < 0 && !songs_.empty()) {
    current_row_ = 0;
  }
  MaybeAutoSort();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::AppendSongs(const SongList &songs) { InsertSongs(static_cast<int>(songs_.size()), songs); }

void Playlist::RemoveRows(const std::vector<int> &rows) {
  if (rows.empty()) {
    return;
  }
  PushUndo();
  std::vector<int> sorted = rows;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  for (int row : sorted) {
    if (row >= 0 && row < static_cast<int>(songs_.size())) {
      songs_.erase(songs_.begin() + row);
      if (current_row_ >= row) {
        current_row_ = std::max(-1, current_row_ - 1);
      }
    }
  }
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Clear() {
  if (songs_.empty()) {
    return;
  }
  PushUndo();
  songs_.clear();
  current_row_ = -1;
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Move(int from, int to) { MoveRows({from}, to); }

void Playlist::MoveRows(const std::vector<int> &rows, int to) {
  if (rows.empty()) {
    return;
  }
  std::vector<int> sorted = rows;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  SongList moving;
  for (int row : sorted) {
    if (row < 0 || row >= row_count()) {
      return;
    }
    moving.push_back(songs_[static_cast<size_t>(row)]);
  }
  const Song playing = current_song();
  int dest = std::clamp(to, 0, row_count());
  PushUndo();
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
    songs_.erase(songs_.begin() + *it);
    if (*it < dest) {
      --dest;
    }
  }
  dest = std::clamp(dest, 0, row_count());
  songs_.insert(songs_.begin() + dest, moving.begin(), moving.end());
  current_row_ = -1;
  if (playing.is_valid() || !playing.url().empty()) {
    for (int i = 0; i < row_count(); ++i) {
      if (songs_[static_cast<size_t>(i)] == playing) {
        current_row_ = i;
        break;
      }
    }
  }
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Shuffle() {
  if (songs_.size() < 2) {
    return;
  }
  PushUndo();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(songs_.begin(), songs_.end(), gen);
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::RemoveDuplicates() {
  PushUndo();
  SongList unique;
  for (const Song &song : songs_) {
    const bool exists = std::any_of(unique.begin(), unique.end(), [&](const Song &other) {
      return !song.url().empty() && other.url() == song.url();
    });
    if (!exists) {
      unique.push_back(song);
    }
  }
  songs_ = std::move(unique);
  if (current_row_ >= static_cast<int>(songs_.size())) {
    current_row_ = songs_.empty() ? -1 : static_cast<int>(songs_.size()) - 1;
  }
  Changed.Emit();
}

void Playlist::InvalidateDeletedSongs() {
  bool changed = false;
  for (Song &song : songs_) {
    if (!PlaylistBehaviour::IsLocalMedia(song)) {
      continue;
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    const bool exists = !path.empty() && FileUtils::Exists(path);
    if (PlaylistBehaviour::ApplyLocalExistence(&song, exists)) {
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

bool Playlist::ApplyValidityOnCurrentSong(const std::string &url, bool valid) {
  (void)url;
  if (current_row_ < 0 || current_row_ >= row_count()) {
    return false;
  }
  if (PlaylistBehaviour::ApplyValidity(&songs_[static_cast<size_t>(current_row_)], valid)) {
    Changed.Emit();
  }
  return true;
}

void Playlist::SetSort(PlaylistColumn column, bool descending) {
  sort_column_ = column;
  sort_descending_ = descending;
}

void Playlist::SortNow() {
  if (sort_column_ == PlaylistColumn::Count) {
    return;
  }
  PushUndo();
  SortInPlace();
  Changed.Emit();
}

void Playlist::MaybeAutoSort() {
  if (auto_sort_ && sort_column_ != PlaylistColumn::Count) {
    SortInPlace();
  }
}

void Playlist::SortInPlace() {
  if (sort_column_ == PlaylistColumn::Count || songs_.size() < 2) {
    return;
  }
  const Song playing = current_song();
  const bool numeric = PlaylistBehaviour::ColumnIsNumeric(sort_column_);
  const PlaylistColumn column = sort_column_;
  const bool descending = sort_descending_;
  std::stable_sort(songs_.begin(), songs_.end(), [column, numeric, descending](const Song &a, const Song &b) {
    return PlaylistBehaviour::LessThanText(PlaylistDelegates::ColumnText(a, column), PlaylistDelegates::ColumnText(b, column), numeric,
                                           descending);
  });
  current_row_ = -1;
  if (playing.is_valid() || !playing.url().empty()) {
    for (int i = 0; i < row_count(); ++i) {
      if (songs_[static_cast<size_t>(i)] == playing) {
        current_row_ = i;
        break;
      }
    }
  }
}

void Playlist::RemoveUnavailable() {
  PushUndo();
  songs_.erase(std::remove_if(songs_.begin(), songs_.end(),
                              [](const Song &song) {
                                const std::string &url = song.url();
                                if (url.empty()) {
                                  return false;
                                }
                                const auto scheme = url.find("://");
                                if (scheme != std::string::npos && url.rfind("file://", 0) != 0) {
                                  return false;
                                }
                                const std::string path = FileUtils::PathFromUri(url);
                                return !path.empty() && !FileUtils::Exists(path);
                              }),
               songs_.end());
  if (current_row_ >= static_cast<int>(songs_.size())) {
    current_row_ = songs_.empty() ? -1 : static_cast<int>(songs_.size()) - 1;
  }
  Changed.Emit();
}

void Playlist::RenumberTracks() {
  PushUndo();
  for (size_t i = 0; i < songs_.size(); ++i) {
    songs_[i].set_track(static_cast<int>(i) + 1);
  }
  Changed.Emit();
}

void Playlist::RateCurrentSong(float rating) {
  if (current_row_ < 0 || current_row_ >= static_cast<int>(songs_.size())) {
    return;
  }
  PushUndo();
  songs_[static_cast<size_t>(current_row_)].set_rating(rating);
  Changed.Emit();
}

void Playlist::SkipTracks(const std::vector<int> &rows) {
  bool all_skipped = true;
  for (int row : rows) {
    if (row >= 0 && row < row_count() && !songs_[static_cast<size_t>(row)].skipped()) {
      all_skipped = false;
      break;
    }
  }
  PushUndo();
  for (int row : rows) {
    if (row >= 0 && row < row_count()) {
      songs_[static_cast<size_t>(row)].set_skipped(!all_skipped);
    }
  }
  Changed.Emit();
}

void Playlist::ReplaceRow(int row, const Song &song) {
  if (row < 0 || row >= row_count()) {
    return;
  }
  PushUndo();
  const bool skipped = songs_[static_cast<size_t>(row)].skipped();
  songs_[static_cast<size_t>(row)] = song;
  songs_[static_cast<size_t>(row)].set_skipped(skipped);
  Changed.Emit();
}

void Playlist::SetFilterString(const std::string &filter) { filter_string_ = filter; }

void Playlist::UpdateSongsByUrl(const Song &song) {
  if (song.url().empty()) {
    return;
  }
  bool changed = false;
  for (int i = 0; i < row_count(); ++i) {
    if (songs_[static_cast<size_t>(i)].url() == song.url()) {
      const bool skipped = songs_[static_cast<size_t>(i)].skipped();
      songs_[static_cast<size_t>(i)] = song;
      songs_[static_cast<size_t>(i)].set_skipped(skipped);
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

bool Playlist::SetColumnValue(int row, PlaylistColumn column, const std::string &value) {
  return SetColumnValues({row}, column, value) > 0;
}

int Playlist::SetColumnValues(const std::vector<int> &rows, PlaylistColumn column, const std::string &value) {
  if (!PlaylistDelegates::ColumnIsEditable(column)) {
    return 0;
  }
  PushUndo();
  int updated = 0;
  for (int row : rows) {
    if (row < 0 || row >= row_count()) {
      continue;
    }
    Song song = songs_[static_cast<size_t>(row)];
    if (!PlaylistDelegates::SetColumnValue(song, column, value)) {
      continue;
    }
    const bool skipped = songs_[static_cast<size_t>(row)].skipped();
    songs_[static_cast<size_t>(row)] = song;
    songs_[static_cast<size_t>(row)].set_skipped(skipped);
    ++updated;
  }
  if (updated == 0) {
    undo_.pop_back();
    return 0;
  }
  Changed.Emit();
  return updated;
}

void Playlist::ReloadRow(int row, TagReader *tagreader) {
  if (!tagreader || row < 0 || row >= row_count()) {
    return;
  }
  const std::string path = FileUtils::PathFromUri(songs_[static_cast<size_t>(row)].url());
  if (path.empty() || !FileUtils::Exists(path)) {
    return;
  }
  Song updated = tagreader->ReadFile(path);
  if (!updated.is_valid()) {
    return;
  }
  updated.set_id(songs_[static_cast<size_t>(row)].id());
  updated.set_skipped(songs_[static_cast<size_t>(row)].skipped());
  PushUndo();
  songs_[static_cast<size_t>(row)] = updated;
  Changed.Emit();
}

void Playlist::SetSequenceMode(SequenceMode mode) {
  mode_ = mode;
  switch (mode) {
    case SequenceMode::RepeatTrack:
      repeat_mode_ = PlaylistSequence::RepeatMode::Track;
      break;
    case SequenceMode::RepeatAll:
      repeat_mode_ = PlaylistSequence::RepeatMode::Playlist;
      break;
    case SequenceMode::Shuffle:
      shuffle_mode_ = PlaylistSequence::ShuffleMode::All;
      break;
    case SequenceMode::AlbumShuffle:
      shuffle_mode_ = PlaylistSequence::ShuffleMode::InsideAlbum;
      break;
    case SequenceMode::Sequential:
      repeat_mode_ = PlaylistSequence::RepeatMode::Off;
      break;
    case SequenceMode::Dynamic:
      break;
  }
}

void Playlist::SetRepeatMode(PlaylistSequence::RepeatMode mode) {
  repeat_mode_ = mode;
  if (mode == PlaylistSequence::RepeatMode::Track) {
    mode_ = SequenceMode::RepeatTrack;
  } else if (mode == PlaylistSequence::RepeatMode::Playlist) {
    mode_ = SequenceMode::RepeatAll;
  } else if (mode == PlaylistSequence::RepeatMode::Off) {
    mode_ = SequenceMode::Sequential;
  }
}

void Playlist::SetShuffleMode(PlaylistSequence::ShuffleMode mode) {
  shuffle_mode_ = mode;
  if (mode == PlaylistSequence::ShuffleMode::All) {
    mode_ = SequenceMode::Shuffle;
  } else if (mode == PlaylistSequence::ShuffleMode::InsideAlbum) {
    mode_ = SequenceMode::AlbumShuffle;
  }
  RebuildVirtualItems();
}

void Playlist::Reshuffle(unsigned seed) { RebuildVirtualItems(seed == 0 ? std::random_device{}() : seed); }

void Playlist::SyncVirtualIndex() {
  current_virtual_index_ = -1;
  for (int i = 0; i < static_cast<int>(virtual_items_.size()); ++i) {
    if (virtual_items_[static_cast<size_t>(i)] == current_row_) {
      current_virtual_index_ = i;
      return;
    }
  }
}

bool Playlist::SameAlbum(int left, int right) const {
  return PlaylistShuffle::AlbumKey(song(left)) == PlaylistShuffle::AlbumKey(song(right));
}

void Playlist::RebuildVirtualItems(unsigned seed) {
  const int n = row_count();
  if (seed == 0 && shuffle_mode_ != PlaylistSequence::ShuffleMode::Off) {
    seed = std::random_device{}();
  }
  std::vector<std::string> keys;
  keys.reserve(static_cast<size_t>(n));
  switch (shuffle_mode_) {
    case PlaylistSequence::ShuffleMode::All:
    case PlaylistSequence::ShuffleMode::InsideAlbum:
      virtual_items_ = PlaylistShuffle::ShuffleAll(n, seed, current_row_);
      break;
    case PlaylistSequence::ShuffleMode::Albums:
      for (int i = 0; i < n; ++i) {
        keys.push_back(PlaylistShuffle::AlbumKey(song(i)));
      }
      virtual_items_ = PlaylistShuffle::ShuffleByKey(keys, seed, current_row_);
      break;
    case PlaylistSequence::ShuffleMode::Grouping:
      for (int i = 0; i < n; ++i) {
        keys.push_back(PlaylistShuffle::GroupingKey(song(i)));
      }
      virtual_items_ = PlaylistShuffle::ShuffleByKey(keys, seed, current_row_);
      break;
    case PlaylistSequence::ShuffleMode::Off:
    default:
      virtual_items_ = PlaylistShuffle::Identity(n);
      break;
  }
  SyncVirtualIndex();
}

int Playlist::NextIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  if (mode_ == SequenceMode::RepeatTrack || repeat_mode_ == PlaylistSequence::RepeatMode::Track) {
    return current_row_;
  }
  if (static_cast<int>(virtual_items_.size()) != row_count()) {
    const_cast<Playlist *>(this)->RebuildVirtualItems();
  }
  const bool wrap = mode_ == SequenceMode::RepeatAll || repeat_mode_ == PlaylistSequence::RepeatMode::Playlist;
  const bool album_only = repeat_mode_ == PlaylistSequence::RepeatMode::Album ||
                          shuffle_mode_ == PlaylistSequence::ShuffleMode::InsideAlbum;
  const int n = static_cast<int>(virtual_items_.size());
  const int start = current_virtual_index_ >= 0 ? current_virtual_index_ : -1;
  auto accept = [&](int virt) -> int {
    if (virt < 0 || virt >= n) {
      return -1;
    }
    const int row = virtual_items_[static_cast<size_t>(virt)];
    const Song candidate = song(row);
    if (candidate.skipped()) {
      return -1;
    }
    if (!filter_string_.empty()) {
      PlaylistFilter filter;
      filter.SetFilterString(filter_string_);
      if (!filter.Accepts(candidate)) {
        return -1;
      }
    }
    if (album_only && current_row_ >= 0 && !SameAlbum(row, current_row_)) {
      return -1;
    }
    return row;
  };
  for (int i = start + 1; i < n; ++i) {
    const int row = accept(i);
    if (row >= 0) {
      return row;
    }
  }
  if (wrap || album_only) {
    for (int i = 0; i < n; ++i) {
      const int row = accept(i);
      if (row >= 0 && row != current_row_) {
        return row;
      }
    }
    return current_row_;
  }
  return -1;
}

int Playlist::PreviousIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  if (static_cast<int>(virtual_items_.size()) != row_count()) {
    const_cast<Playlist *>(this)->RebuildVirtualItems();
  }
  const bool wrap = mode_ == SequenceMode::RepeatAll || repeat_mode_ == PlaylistSequence::RepeatMode::Playlist;
  const bool album_only = repeat_mode_ == PlaylistSequence::RepeatMode::Album ||
                          shuffle_mode_ == PlaylistSequence::ShuffleMode::InsideAlbum;
  const int n = static_cast<int>(virtual_items_.size());
  const int start = current_virtual_index_ >= 0 ? current_virtual_index_ : 0;
  auto accept = [&](int virt) -> int {
    if (virt < 0 || virt >= n) {
      return -1;
    }
    const int row = virtual_items_[static_cast<size_t>(virt)];
    const Song candidate = song(row);
    if (candidate.skipped()) {
      return -1;
    }
    if (!filter_string_.empty()) {
      PlaylistFilter filter;
      filter.SetFilterString(filter_string_);
      if (!filter.Accepts(candidate)) {
        return -1;
      }
    }
    if (album_only && current_row_ >= 0 && !SameAlbum(row, current_row_)) {
      return -1;
    }
    return row;
  };
  for (int i = start - 1; i >= 0; --i) {
    const int row = accept(i);
    if (row >= 0) {
      return row;
    }
  }
  if (!wrap) {
    return current_row_;
  }
  for (int i = n - 1; i >= 0; --i) {
    const int row = accept(i);
    if (row >= 0) {
      return row;
    }
  }
  return current_row_;
}

void Playlist::Next() {
  const int next = NextIndex();
  if (next >= 0) {
    set_current_row(next);
  }
}

void Playlist::Previous() {
  const int previous = PreviousIndex();
  if (previous >= 0) {
    set_current_row(previous);
  }
}

int64_t Playlist::total_length_nanosec() const {
  return std::accumulate(songs_.begin(), songs_.end(), int64_t{0},
                         [](int64_t total, const Song &song) { return total + std::max<int64_t>(0, song.length_nanosec()); });
}

void Playlist::SetDynamic(bool dynamic, const SmartPlaylistSearch &search) {
  dynamic_ = dynamic;
  dynamic_search_ = search;
  if (dynamic) {
    mode_ = SequenceMode::Dynamic;
  } else if (mode_ == SequenceMode::Dynamic) {
    mode_ = SequenceMode::Sequential;
  }
}

void Playlist::RefillDynamic(const SongList &pool, bool force) {
  if (!dynamic_) {
    return;
  }
  const int upcoming = current_row_ < 0 ? row_count() : row_count() - current_row_ - 1;
  if (!force && upcoming >= 15) {
    return;
  }
  const int want = force ? 15 : std::max(0, 15 - upcoming);
  if (want <= 0) {
    return;
  }
  SongList candidates = dynamic_search_.Search(pool);
  SongList extra;
  for (const Song &song : candidates) {
    bool seen = false;
    for (const Song &existing : songs_) {
      if (existing.url() == song.url()) {
        seen = true;
        break;
      }
    }
    if (!seen) {
      extra.push_back(song);
    }
    if (static_cast<int>(extra.size()) >= want) {
      break;
    }
  }
  if (!extra.empty()) {
    AppendSongs(extra);
  }
}

void Playlist::ExpandDynamic(const SongList &pool) { RefillDynamic(pool, true); }

void Playlist::RepopulateDynamic(const SongList &pool) {
  if (!dynamic_) {
    return;
  }
  std::vector<int> after;
  for (int i = current_row_ + 1; i < row_count(); ++i) {
    after.push_back(i);
  }
  if (!after.empty()) {
    RemoveRows(after);
  }
  RefillDynamic(pool, true);
}
