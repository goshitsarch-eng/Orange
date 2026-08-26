#ifndef STRAWBERRY_QUEUE_H
#define STRAWBERRY_QUEUE_H

#include "core/signal.h"
#include "core/song.h"
#include "queue/queuerows.h"

#include <vector>

class Queue {
 public:
  void Append(const Song &song);
  void Append(const Song &song, int playlist_id, int playlist_row);
  void InsertNext(const Song &song);
  void InsertNext(const Song &song, int playlist_id, int playlist_row);
  void Insert(int index, const Song &song);
  void Insert(int index, const Song &song, int playlist_id, int playlist_row);
  void Insert(int index, const SongList &songs);
  void Insert(int index, const SongList &songs, const std::vector<QueueRows::Source> &sources);
  Song TakeNext();
  QueueRows::Source PeekSource() const;
  QueueRows::Source last_taken_source() const { return last_taken_; }
  void Remove(int index);
  void RemoveRows(const std::vector<int> &indexes);
  void RemoveSong(const Song &song);
  void TogglePlaylistRow(int playlist_id, int playlist_row, const Song &song);
  bool Contains(const Song &song) const;
  bool ContainsPlaylistRow(int playlist_id, int playlist_row) const;
  int PositionForPlaylistRow(int playlist_id, int playlist_row) const;
  void RemapAfterPlaylistRemove(int playlist_id, const std::vector<int> &removed);
  void RemapAfterPlaylistInsert(int playlist_id, int at, int count);
  void Clear();
  void Move(int from, int to);
  void MoveRows(const std::vector<int> &from, int to);
  const SongList &songs() const { return songs_; }
  const std::vector<QueueRows::Source> &sources() const { return sources_; }
  bool empty() const { return songs_.empty(); }
  int size() const { return static_cast<int>(songs_.size()); }

  Signal<> Changed;

 private:
  void SyncSources();

  SongList songs_;
  std::vector<QueueRows::Source> sources_;
  QueueRows::Source last_taken_;
};

#endif  // STRAWBERRY_QUEUE_H
