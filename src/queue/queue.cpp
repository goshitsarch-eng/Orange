#include "queue/queue.h"

#include <algorithm>

void Queue::Append(const Song &song) {
  songs_.push_back(song);
  Changed.Emit();
}

void Queue::InsertNext(const Song &song) {
  songs_.insert(songs_.begin(), song);
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
