#include "queue/queue.h"

#include <algorithm>
#include <functional>

void Queue::SyncSources() {
  if (sources_.size() < songs_.size()) {
    sources_.resize(songs_.size());
  } else if (sources_.size() > songs_.size()) {
    sources_.resize(songs_.size());
  }
}

void Queue::Append(const Song &song) { Append(song, -1, -1); }

void Queue::Append(const Song &song, int playlist_id, int playlist_row) {
  songs_.push_back(song);
  sources_.push_back({playlist_id, playlist_row});
  Changed.Emit();
}

void Queue::InsertNext(const Song &song) { InsertNext(song, -1, -1); }

void Queue::InsertNext(const Song &song, int playlist_id, int playlist_row) {
  songs_.insert(songs_.begin(), song);
  sources_.insert(sources_.begin(), {playlist_id, playlist_row});
  Changed.Emit();
}

void Queue::Insert(int index, const Song &song) { Insert(index, song, -1, -1); }

void Queue::Insert(int index, const Song &song, int playlist_id, int playlist_row) {
  Insert(index, SongList{song}, std::vector<QueueRows::Source>{{playlist_id, playlist_row}});
}

void Queue::Insert(int index, const SongList &songs) { Insert(index, songs, {}); }

void Queue::Insert(int index, const SongList &songs, const std::vector<QueueRows::Source> &sources) {
  if (songs.empty()) {
    return;
  }
  SyncSources();
  if (index < 0) {
    index = 0;
  }
  if (index > size()) {
    index = size();
  }
  songs_.insert(songs_.begin() + index, songs.begin(), songs.end());
  std::vector<QueueRows::Source> padded = sources;
  padded.resize(songs.size());
  sources_.insert(sources_.begin() + index, padded.begin(), padded.end());
  Changed.Emit();
}

Song Queue::TakeNext() {
  SyncSources();
  last_taken_ = {};
  if (songs_.empty()) {
    return Song();
  }
  Song song = songs_.front();
  last_taken_ = sources_.front();
  songs_.erase(songs_.begin());
  sources_.erase(sources_.begin());
  Changed.Emit();
  return song;
}

QueueRows::Source Queue::PeekSource() const { return sources_.empty() ? QueueRows::Source{} : sources_.front(); }

void Queue::Remove(int index) {
  SyncSources();
  if (index >= 0 && index < static_cast<int>(songs_.size())) {
    songs_.erase(songs_.begin() + index);
    sources_.erase(sources_.begin() + index);
    Changed.Emit();
  }
}

void Queue::RemoveRows(const std::vector<int> &indexes) {
  SyncSources();
  std::vector<int> sorted = indexes;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  bool changed = false;
  for (int index : sorted) {
    if (index >= 0 && index < size()) {
      songs_.erase(songs_.begin() + index);
      sources_.erase(sources_.begin() + index);
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

void Queue::RemoveSong(const Song &song) {
  SyncSources();
  bool changed = false;
  for (int i = size() - 1; i >= 0; --i) {
    if (songs_[static_cast<size_t>(i)].url() == song.url()) {
      songs_.erase(songs_.begin() + i);
      sources_.erase(sources_.begin() + i);
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

void Queue::TogglePlaylistRow(int playlist_id, int playlist_row, const Song &song) {
  SyncSources();
  const int position = PositionForPlaylistRow(playlist_id, playlist_row);
  if (position > 0) {
    Remove(position - 1);
    return;
  }
  Append(song, playlist_id, playlist_row);
}

bool Queue::Contains(const Song &song) const {
  return std::any_of(songs_.begin(), songs_.end(), [&](const Song &other) { return other.url() == song.url(); });
}

bool Queue::ContainsPlaylistRow(int playlist_id, int playlist_row) const {
  return PositionForPlaylistRow(playlist_id, playlist_row) > 0;
}

int Queue::PositionForPlaylistRow(int playlist_id, int playlist_row) const {
  return QueueRows::PositionForRow(sources_, playlist_id, playlist_row);
}

void Queue::RemapAfterPlaylistRemove(int playlist_id, const std::vector<int> &removed) {
  SyncSources();
  const auto remapped = QueueRows::AfterRemove(sources_, playlist_id, removed);
  if (remapped.size() != sources_.size()) {
    SongList kept;
    kept.reserve(remapped.size());
    std::vector<int> sorted = removed;
    std::sort(sorted.begin(), sorted.end());
    for (size_t i = 0; i < sources_.size(); ++i) {
      const QueueRows::Source &source = sources_[i];
      const bool drop = source.valid() && source.playlist_id == playlist_id &&
                        std::binary_search(sorted.begin(), sorted.end(), source.row);
      if (!drop) {
        kept.push_back(songs_[i]);
      }
    }
    songs_ = std::move(kept);
  }
  sources_ = remapped;
  Changed.Emit();
}

void Queue::RemapAfterPlaylistInsert(int playlist_id, int at, int count) {
  SyncSources();
  sources_ = QueueRows::AfterInsert(sources_, playlist_id, at, count);
}

void Queue::Clear() {
  songs_.clear();
  sources_.clear();
  last_taken_ = {};
  Changed.Emit();
}

void Queue::Move(int from, int to) {
  SyncSources();
  if (from < 0 || to < 0 || from >= size() || to >= size()) {
    return;
  }
  Song song = songs_[static_cast<size_t>(from)];
  QueueRows::Source source = sources_[static_cast<size_t>(from)];
  songs_.erase(songs_.begin() + from);
  sources_.erase(sources_.begin() + from);
  songs_.insert(songs_.begin() + to, song);
  sources_.insert(sources_.begin() + to, source);
  Changed.Emit();
}

void Queue::MoveRows(const std::vector<int> &from, int to) {
  SyncSources();
  if (from.empty()) {
    return;
  }
  std::vector<int> sorted = from;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  SongList moving;
  std::vector<QueueRows::Source> moving_sources;
  for (int row : sorted) {
    if (row < 0 || row >= size()) {
      return;
    }
    moving.push_back(songs_[static_cast<size_t>(row)]);
    moving_sources.push_back(sources_[static_cast<size_t>(row)]);
  }
  int dest = to;
  if (dest < 0) {
    dest = 0;
  }
  if (dest > size()) {
    dest = size();
  }
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
    songs_.erase(songs_.begin() + *it);
    sources_.erase(sources_.begin() + *it);
    if (*it < dest) {
      --dest;
    }
  }
  if (dest < 0) {
    dest = 0;
  }
  if (dest > size()) {
    dest = size();
  }
  songs_.insert(songs_.begin() + dest, moving.begin(), moving.end());
  sources_.insert(sources_.begin() + dest, moving_sources.begin(), moving_sources.end());
  Changed.Emit();
}
