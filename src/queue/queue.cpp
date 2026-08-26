#include "queue/queue.h"

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
