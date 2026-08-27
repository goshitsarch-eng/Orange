#ifndef STRAWBERRY_PLAYLIST_H
#define STRAWBERRY_PLAYLIST_H

#include "core/signal.h"
#include "core/song.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistfilter.h"
#include "playlist/playlistsequence.h"
#include "queue/queue.h"
#include "smartplaylists/playlistgenerator.h"
#include "smartplaylists/smartplaylist.h"

#include <memory>
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
  int last_played_row() const { return last_played_row_; }
  void set_current_row(int row);
  void UpdateScrobblePoint(int64_t seek_point_nanosec = 0);
  int64_t scrobble_point_nanosec() const { return scrobble_point_nanosec_; }
  bool scrobbled() const { return scrobbled_; }
  void set_scrobbled(bool scrobbled) { scrobbled_ = scrobbled; }
  bool PatchSongById(const Song &song);
  Song current_song() const;
  Song song(int row) const;
  int PeekNextRow() const { return NextIndex(); }
  int PeekPreviousRow() const { return PreviousIndex(); }
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
  void InvalidateDeletedSongs(class TagReader *tagreader = nullptr);
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
  bool MergeFromEngine(const Song &engine);
  bool UpdateRowMetadata(int row, const Song &engine);
  PlaylistSequence::RepeatMode repeat_mode() const { return repeat_mode_; }
  PlaylistSequence::ShuffleMode shuffle_mode() const { return shuffle_mode_; }
  void SetDynamic(bool dynamic, const SmartPlaylistSearch &search = {});
  void SetDynamicGenerator(std::shared_ptr<PlaylistGenerator> generator);
  std::shared_ptr<PlaylistGenerator> dynamic_generator() const { return dynamic_generator_; }
  bool is_dynamic() const { return dynamic_; }
  const SmartPlaylistSearch &dynamic_search() const { return dynamic_search_; }
  void RefillDynamic(const SongList &pool, bool force = false);
  void ExpandDynamic(const SongList &pool);
  void RepopulateDynamic(const SongList &pool);
  void RemoveItemsNotInQueue();
  void ApplyDiscoveredArt(const Song &playing, const std::string &discovered);
  void set_stop_after_row(int row);
  void ToggleStopAfter(int row);
  int stop_after_row() const { return stop_after_row_; }
  Queue *queue() { return &queue_; }
  const Queue *queue() const { return &queue_; }
  void BeginLoad() { loading_ = true; }
  void EndLoad() { loading_ = false; }
  bool loading() const { return loading_; }
  std::string UuidAt(int row) const;
  const std::vector<std::string> &uuids() const { return uuids_; }
  void SetRowUuids(const std::vector<std::string> &uuids);
  void EnsureUuids();

  int64_t total_length_nanosec() const;

  Signal<> Changed;
  Signal<int> CurrentChanged;
  Signal<> RepeatModeChanged;
  Signal<> ShuffleModeChanged;

 private:
  struct Snapshot {
    SongList songs;
    std::vector<std::string> uuids;
    int current_row = -1;
  };

  int NextIndex() const;
  int PreviousIndex() const;
  void RebuildVirtualItems(unsigned seed = 0);
  void SyncVirtualIndex();
  bool SameAlbum(int left, int right) const;
  void PushUndo();
  void MaybeRecordUndo(int item_count);
  void RemoveRowsInternal(const std::vector<int> &rows, bool record_undo);
  void MaintainDynamicAfterAdvance(int old_row);
  void InsertDynamicMore(int count);
  void MaybeAutoSort();
  void SortInPlace();

  int id_ = -1;
  std::string name_ = "Playlist";
  std::string ui_path_;
  bool favorite_ = false;
  SongList songs_;
  int current_row_ = -1;
  int last_played_row_ = -1;
  SequenceMode mode_ = SequenceMode::Sequential;
  PlaylistSequence::RepeatMode repeat_mode_ = PlaylistSequence::RepeatMode::Off;
  PlaylistSequence::ShuffleMode shuffle_mode_ = PlaylistSequence::ShuffleMode::Off;
  std::vector<Snapshot> undo_;
  std::vector<Snapshot> redo_;
  bool dynamic_ = false;
  SmartPlaylistSearch dynamic_search_;
  std::shared_ptr<PlaylistGenerator> dynamic_generator_;
  bool auto_sort_ = false;
  PlaylistColumn sort_column_ = PlaylistColumn::Count;
  bool sort_descending_ = false;
  std::vector<int> virtual_items_;
  int current_virtual_index_ = -1;
  std::string filter_string_;
  PlaylistFilter filter_;
  std::vector<int> played_indexes_;
  int stop_after_row_ = -1;
  int64_t scrobble_point_nanosec_ = -1;
  bool scrobbled_ = false;
  bool loading_ = false;
  Queue queue_;
  std::vector<std::string> uuids_;
};

#endif  // STRAWBERRY_PLAYLIST_H
