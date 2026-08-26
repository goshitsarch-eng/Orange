#include "queue/queue.h"

#include <algorithm>
#include <functional>

void Queue::Append(const Song &song) {
  songs_.push_back(song);
  Changed.Emit();
}

void Queue::InsertNext(const Song &song) {
  songs_.insert(songs_.begin(), song);
  Changed.Emit();
}

void Queue::Insert(int index, const Song &song) { Insert(index, SongList{song}); }

void Queue::Insert(int index, const SongList &songs) {
  if (songs.empty()) {
    return;
  }
  if (index < 0) {
    index = 0;
  }
  if (index > size()) {
    index = size();
  }
  songs_.insert(songs_.begin() + index, songs.begin(), songs.end());
  Changed.Emit();
}

Song Queue::TakeNext() {
  if (songs_.empty()) {
    return Song();
  }
  Song song = songs_.front();
  songs_.erase(songs_.begin());
  Changed.Emit();
  return song;
}

void Queue::Remove(int index) {
  if (index >= 0 && index < static_cast<int>(songs_.size())) {
    songs_.erase(songs_.begin() + index);
    Changed.Emit();
  }
}

void Queue::RemoveRows(const std::vector<int> &indexes) {
  std::vector<int> sorted = indexes;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  bool changed = false;
  for (int index : sorted) {
    if (index >= 0 && index < size()) {
      songs_.erase(songs_.begin() + index);
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

void Queue::RemoveSong(const Song &song) {
  songs_.erase(std::remove_if(songs_.begin(), songs_.end(), [&](const Song &other) { return other.url() == song.url(); }), songs_.end());
  Changed.Emit();
}

bool Queue::Contains(const Song &song) const {
  return std::any_of(songs_.begin(), songs_.end(), [&](const Song &other) { return other.url() == song.url(); });
}

void Queue::Clear() {
  songs_.clear();
  Changed.Emit();
}

void Queue::Move(int from, int to) {
  if (from < 0 || to < 0 || from >= size() || to >= size()) {
    return;
  }
  Song song = songs_[static_cast<size_t>(from)];
  songs_.erase(songs_.begin() + from);
  songs_.insert(songs_.begin() + to, song);
  Changed.Emit();
}

void Queue::MoveRows(const std::vector<int> &from, int to) {
  if (from.empty()) {
    return;
  }
  std::vector<int> sorted = from;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  SongList moving;
  for (int row : sorted) {
    if (row < 0 || row >= size()) {
      return;
    }
    moving.push_back(songs_[static_cast<size_t>(row)]);
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
  Changed.Emit();
}
