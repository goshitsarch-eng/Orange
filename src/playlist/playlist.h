#ifndef STRAWBERRY_PLAYLIST_H
#define STRAWBERRY_PLAYLIST_H

#include "core/signal.h"
#include "core/song.h"
#include "playlist/playlistdelegates.h"
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
  const std::string &ui_path() const { return ui_path_; }
  void set_ui_path(const std::string &path) { ui_path_ = path; }

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
  void MoveRows(const std::vector<int> &rows, int to);
  void Shuffle();
  void RemoveDuplicates();
  void RemoveUnavailable();
  void InvalidateDeletedSongs();
  bool ApplyValidityOnCurrentSong(const std::string &url, bool valid);
  void set_auto_sort(bool auto_sort) { auto_sort_ = auto_sort; }
  bool auto_sort() const { return auto_sort_; }
  void SetSort(PlaylistColumn column, bool descending);
  PlaylistColumn sort_column() const { return sort_column_; }
  bool sort_descending() const { return sort_descending_; }
  void SortNow();
  void RenumberTracks();
  void RateCurrentSong(float rating);
  void SkipTracks(const std::vector<int> &rows);
  void ReplaceRow(int row, const Song &song);
  bool SetColumnValue(int row, PlaylistColumn column, const std::string &value);
  int SetColumnValues(const std::vector<int> &rows, PlaylistColumn column, const std::string &value);
  void ReloadRow(int row, class TagReader *tagreader);
  void ReplaceSongs(const SongList &songs);
  void Undo();
  void Redo();
  bool CanUndo() const { return !undo_.empty(); }
  bool CanRedo() const { return !redo_.empty(); }
  void Next();
  void Previous();
  void RecordAndSetCurrentRow(int row);
  const std::vector<int> &played_indexes() const { return played_indexes_; }
  void Reshuffle(unsigned seed = 0);
  const std::vector<int> &virtual_items() const { return virtual_items_; }
  void SetSequenceMode(SequenceMode mode);
  SequenceMode sequence_mode() const { return mode_; }
  void SetRepeatMode(PlaylistSequence::RepeatMode mode);
  void SetShuffleMode(PlaylistSequence::ShuffleMode mode);
  void SetFilterString(const std::string &filter);
  const std::string &filter_string() const { return filter_string_; }
  void UpdateSongsByUrl(const Song &song);
  PlaylistSequence::RepeatMode repeat_mode() const { return repeat_mode_; }
  PlaylistSequence::ShuffleMode shuffle_mode() const { return shuffle_mode_; }
  void SetDynamic(bool dynamic, const SmartPlaylistSearch &search = {});
  bool is_dynamic() const { return dynamic_; }
  const SmartPlaylistSearch &dynamic_search() const { return dynamic_search_; }
  void RefillDynamic(const SongList &pool, bool force = false);
  void ExpandDynamic(const SongList &pool);
  void RepopulateDynamic(const SongList &pool);

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
  void RebuildVirtualItems(unsigned seed = 0);
  void SyncVirtualIndex();
  bool SameAlbum(int left, int right) const;
  void PushUndo();
  void MaybeAutoSort();
  void SortInPlace();

  int id_ = -1;
  std::string name_ = "Playlist";
  std::string ui_path_;
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
  bool auto_sort_ = false;
  PlaylistColumn sort_column_ = PlaylistColumn::Count;
  bool sort_descending_ = false;
  std::vector<int> virtual_items_;
  int current_virtual_index_ = -1;
  std::string filter_string_;
  std::vector<int> played_indexes_;
};

#endif  // STRAWBERRY_PLAYLIST_H
