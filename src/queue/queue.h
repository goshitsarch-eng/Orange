#ifndef STRAWBERRY_QUEUE_H
#define STRAWBERRY_QUEUE_H

#include "core/signal.h"
#include "core/song.h"

#include <vector>

class Queue {
 public:
  void Append(const Song &song);
  void InsertNext(const Song &song);
  Song TakeNext();
  void Remove(int index);
  void RemoveSong(const Song &song);
  bool Contains(const Song &song) const;
  void Clear();
  void Move(int from, int to);
  const SongList &songs() const { return songs_; }
  bool empty() const { return songs_.empty(); }
  int size() const { return static_cast<int>(songs_.size()); }

  Signal<> Changed;

 private:
  SongList songs_;
};

#endif  // STRAWBERRY_QUEUE_H
