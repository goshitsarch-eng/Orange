#include "playlist/playlist.h"

#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <numeric>
#include <random>

Playlist::Playlist() = default;

void Playlist::set_current_row(int row) {
  if (songs_.empty()) {
    current_row_ = -1;
    CurrentChanged.Emit(current_row_);
    return;
  }
  current_row_ = std::clamp(row, 0, static_cast<int>(songs_.size()) - 1);
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
  Changed.Emit();
}

void Playlist::Clear() {
  if (songs_.empty()) {
    return;
  }
  PushUndo();
  songs_.clear();
  current_row_ = -1;
  Changed.Emit();
}

void Playlist::Move(int from, int to) {
  if (from < 0 || to < 0 || from >= static_cast<int>(songs_.size()) || to >= static_cast<int>(songs_.size()) || from == to) {
    return;
  }
  PushUndo();
  Song song = songs_[static_cast<size_t>(from)];
  songs_.erase(songs_.begin() + from);
  songs_.insert(songs_.begin() + to, song);
  if (current_row_ == from) {
    current_row_ = to;
  }
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
}

int Playlist::NextIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  if (mode_ == SequenceMode::RepeatTrack || repeat_mode_ == PlaylistSequence::RepeatMode::Track) {
    return current_row_;
  }
  const bool wrap = mode_ == SequenceMode::RepeatAll || repeat_mode_ == PlaylistSequence::RepeatMode::Playlist;
  if (repeat_mode_ == PlaylistSequence::RepeatMode::Album && current_row_ >= 0) {
    const std::string album = song(current_row_).album();
    for (int i = 1; i <= row_count(); ++i) {
      const int row = (current_row_ + i) % row_count();
      if (!song(row).skipped() && song(row).album() == album) {
        return row;
      }
    }
  }
  const int start = current_row_ < 0 ? -1 : current_row_;
  const int n = row_count();
  for (int i = 1; i <= n; ++i) {
    int row = start + i;
    if (row >= n) {
      if (!wrap) {
        return -1;
      }
      row %= n;
    }
    if (!song(row).skipped()) {
      return row;
    }
  }
  return wrap ? current_row_ : -1;
}

int Playlist::PreviousIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  const bool wrap = mode_ == SequenceMode::RepeatAll;
  const int start = current_row_ < 0 ? 0 : current_row_;
  const int n = row_count();
  for (int i = 1; i <= n; ++i) {
    int row = start - i;
    if (row < 0) {
      if (!wrap) {
        return current_row_;
      }
      row += n;
    }
    if (!song(row).skipped()) {
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
  }
}

void Playlist::RefillDynamic(const SongList &pool) {
  if (!dynamic_) {
    return;
  }
  const int upcoming = current_row_ < 0 ? row_count() : row_count() - current_row_ - 1;
  if (upcoming >= 15) {
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
    if (static_cast<int>(extra.size()) >= 15 - upcoming) {
      break;
    }
  }
  if (!extra.empty()) {
    AppendSongs(extra);
  }
}
