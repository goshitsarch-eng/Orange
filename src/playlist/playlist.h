#ifndef STRAWBERRY_PLAYLIST_H
#define STRAWBERRY_PLAYLIST_H

#include "core/signal.h"
#include "core/song.h"

#include <string>
#include <vector>

class Playlist {
 public:
  enum class SequenceMode { Sequential, RepeatAll, RepeatTrack, Shuffle, AlbumShuffle, Dynamic };
  enum class AutoScroll { Never, Maybe, Always };

  Playlist();

  int id() const { return id_; }
  void set_id(int id) { id_ = id; }
  const std::string &name() const { return name_; }
  void set_name(const std::string &name) { name_ = name; }
  bool favorite() const { return favorite_; }
  void set_favorite(bool favorite) { favorite_ = favorite; }

  const SongList &songs() const { return songs_; }
  int row_count() const { return static_cast<int>(songs_.size()); }
  int current_row() const { return current_row_; }
  void set_current_row(int row);
  Song current_song() const;
  Song song(int row) const;

  void InsertSongs(int row, const SongList &songs);
  void AppendSongs(const SongList &songs);
  void RemoveRows(const std::vector<int> &rows);
  void Clear();
  void Move(int from, int to);
  void Shuffle();
  void Next();
  void Previous();
  void SetSequenceMode(SequenceMode mode) { mode_ = mode; }
  SequenceMode sequence_mode() const { return mode_; }

  int64_t total_length_nanosec() const;

  Signal<> Changed;
  Signal<int> CurrentChanged;

 private:
  int NextIndex() const;
  int PreviousIndex() const;

  int id_ = -1;
  std::string name_ = "Playlist";
  bool favorite_ = false;
  SongList songs_;
  int current_row_ = -1;
  SequenceMode mode_ = SequenceMode::Sequential;
};

#endif  // STRAWBERRY_PLAYLIST_H
