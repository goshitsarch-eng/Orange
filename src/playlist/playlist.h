#ifndef STRAWBERRY_PLAYLIST_H
#define STRAWBERRY_PLAYLIST_H

#include "core/signal.h"
#include "core/song.h"
#include "playlist/playlistsequence.h"
#include "smartplaylists/smartplaylist.h"

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
  int PeekNextRow() const { return NextIndex(); }
  Song PeekNextSong() const;

  void InsertSongs(int row, const SongList &songs);
  void AppendSongs(const SongList &songs);
  void RemoveRows(const std::vector<int> &rows);
  void Clear();
  void Move(int from, int to);
  void Shuffle();
  void RemoveDuplicates();
  void RemoveUnavailable();
  void RenumberTracks();
  void RateCurrentSong(float rating);
  void ReplaceSongs(const SongList &songs);
  void Undo();
  void Redo();
  bool CanUndo() const { return !undo_.empty(); }
  bool CanRedo() const { return !redo_.empty(); }
  void Next();
  void Previous();
  void SetSequenceMode(SequenceMode mode);
  SequenceMode sequence_mode() const { return mode_; }
  void SetRepeatMode(PlaylistSequence::RepeatMode mode);
  void SetShuffleMode(PlaylistSequence::ShuffleMode mode);
  PlaylistSequence::RepeatMode repeat_mode() const { return repeat_mode_; }
  PlaylistSequence::ShuffleMode shuffle_mode() const { return shuffle_mode_; }
  void SetDynamic(bool dynamic, const SmartPlaylistSearch &search = {});
  bool is_dynamic() const { return dynamic_; }
  void RefillDynamic(const SongList &pool);

  int64_t total_length_nanosec() const;

  Signal<> Changed;
  Signal<int> CurrentChanged;

 private:
  struct Snapshot {
    SongList songs;
    int current_row = -1;
  };

  int NextIndex() const;
  int PreviousIndex() const;
  void PushUndo();

  int id_ = -1;
  std::string name_ = "Playlist";
  bool favorite_ = false;
  SongList songs_;
  int current_row_ = -1;
  SequenceMode mode_ = SequenceMode::Sequential;
  PlaylistSequence::RepeatMode repeat_mode_ = PlaylistSequence::RepeatMode::Off;
  PlaylistSequence::ShuffleMode shuffle_mode_ = PlaylistSequence::ShuffleMode::Off;
  std::vector<Snapshot> undo_;
  std::vector<Snapshot> redo_;
  bool dynamic_ = false;
  SmartPlaylistSearch dynamic_search_;
};

#endif  // STRAWBERRY_PLAYLIST_H
