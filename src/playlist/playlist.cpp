#include "playlist/playlist.h"

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

int Playlist::NextIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  switch (mode_) {
    case SequenceMode::RepeatTrack:
      return current_row_;
    case SequenceMode::Shuffle:
    case SequenceMode::AlbumShuffle:
    case SequenceMode::Dynamic:
    case SequenceMode::RepeatAll:
    case SequenceMode::Sequential:
    default:
      if (current_row_ + 1 < static_cast<int>(songs_.size())) {
        return current_row_ + 1;
      }
      return mode_ == SequenceMode::RepeatAll ? 0 : -1;
  }
}

int Playlist::PreviousIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  if (current_row_ > 0) {
    return current_row_ - 1;
  }
  return mode_ == SequenceMode::RepeatAll ? static_cast<int>(songs_.size()) - 1 : current_row_;
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
