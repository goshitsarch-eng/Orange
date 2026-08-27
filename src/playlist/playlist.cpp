#include "playlist/playlist.h"

#include "core/playermetadatasync.h"
#include "playlist/playlistmetadataupdate.h"
#include "playlist/playlistautosort.h"
#include "playlist/playlistitemuuid.h"
#include "playlist/dynamicplaylistmaintenance.h"
#include "playlist/playlistdynamicadvance.h"
#include "playlist/playlistcollectionsync.h"
#include "playlist/playlistremoveitemsnotinqueue.h"
#include "playlist/playlistplayrow.h"
#include "playlist/playlistsaveitem.h"
#include "tagreader/tagreaderclient.h"
#include "tagreader/tagreaderreadfilereply.h"
#include "playlist/playlistlocalartdiscover.h"
#include "playlist/playliststopafter.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistfilter.h"
#include "playlist/playlistfilterindex.h"
#include "playlist/playlistcoverpersist.h"
#include "playlist/playlistqueuedequeue.h"
#include "playlist/playlistreloadrows.h"
#include "playlist/playliststreamstate.h"
#include "playlist/playlistplayed.h"
#include "playlist/playlistshuffle.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistundolimits.h"
#include "scrobbler/scrobblepoint.h"
#include "smartplaylists/playlistgenerator.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <random>

Playlist::Playlist() = default;

Playlist::~Playlist() {
  for (const std::shared_ptr<bool> &alive : pending_save_flags_) {
    if (alive) {
      *alive = false;
    }
  }
}

void Playlist::set_current_row(int row) {
  if (songs_.empty()) {
    current_row_ = -1;
    current_virtual_index_ = -1;
    UpdateScrobblePoint();
    CurrentChanged.Emit(current_row_);
    return;
  }
  const int next_row = std::clamp(row, 0, static_cast<int>(songs_.size()) - 1);
  if (next_row != current_row_) {
    Song *current = current_row_ >= 0 && current_row_ < row_count() ? &songs_[static_cast<size_t>(current_row_)] : nullptr;
    const int peek = PeekNextRow();
    Song *next = peek >= 0 && peek < row_count() ? &songs_[static_cast<size_t>(peek)] : nullptr;
    PlaylistStreamState::ClearForRowChange(current, next);
    if (PlaylistQueueDequeue::ShouldDequeue(id_, next_row, queue_)) {
      queue_.TakeNext();
      Changed.Emit();
    }
  }
  current_row_ = next_row;
  last_played_row_ = PlaylistPlayRow::Remember(current_row_, last_played_row_);
  SyncVirtualIndex();
  UpdateScrobblePoint();
  CurrentChanged.Emit(current_row_);
}

void Playlist::UpdateScrobblePoint(int64_t seek_point_nanosec) {
  scrobble_point_nanosec_ = ScrobblePoint::Compute(current_song().length_nanosec(), seek_point_nanosec);
  scrobbled_ = false;
}

bool Playlist::PatchSongById(const Song &song) {
  if (song.id() <= 0) {
    return false;
  }
  bool changed = false;
  for (Song &existing : songs_) {
    if (!PlaylistCollectionSync::SameCollectionRow(existing, song)) {
      continue;
    }
    const bool skipped = existing.skipped();
    existing = song;
    existing.set_skipped(skipped);
    changed = true;
  }
  if (changed) {
    Changed.Emit();
  }
  return changed;
}

void Playlist::set_stop_after_row(int row) {
  if (row < 0 || row >= row_count()) {
    stop_after_row_ = -1;
  } else {
    stop_after_row_ = row;
  }
  Changed.Emit();
}

void Playlist::ToggleStopAfter(int row) { set_stop_after_row(PlaylistStopAfter::ToggleRow(stop_after_row_, row)); }

Song Playlist::current_song() const { return song(current_row_); }

Song Playlist::song(int row) const {
  if (row < 0 || row >= static_cast<int>(songs_.size())) {
    return Song();
  }
  return songs_[static_cast<size_t>(row)];
}

Song Playlist::PeekNextSong() const { return song(NextIndex()); }

void Playlist::EnsureUuids() {
  if (uuids_.size() < songs_.size()) {
    uuids_.reserve(songs_.size());
    while (uuids_.size() < songs_.size()) {
      uuids_.push_back(PlaylistItemUuid::New());
    }
  } else if (uuids_.size() > songs_.size()) {
    uuids_.resize(songs_.size());
  }
  for (std::string &uuid : uuids_) {
    if (uuid.empty()) {
      uuid = PlaylistItemUuid::New();
    }
  }
}

std::string Playlist::UuidAt(int row) const {
  if (row < 0 || row >= static_cast<int>(uuids_.size())) {
    return {};
  }
  return uuids_[static_cast<size_t>(row)];
}

void Playlist::SetRowUuids(const std::vector<std::string> &uuids) {
  uuids_ = uuids;
  EnsureUuids();
}

void Playlist::PushUndo() {
  EnsureUuids();
  undo_.push_back({songs_, uuids_, current_row_});
  redo_.clear();
  if (static_cast<int>(undo_.size()) > PlaylistUndoLimits::kUndoStackLimit) {
    undo_.erase(undo_.begin());
  }
}

void Playlist::MaybeRecordUndo(int item_count) {
  if (PlaylistUndoLimits::ShouldBypassUndo(item_count)) {
    undo_.clear();
    redo_.clear();
    return;
  }
  PushUndo();
}

void Playlist::Undo() {
  if (undo_.empty()) {
    return;
  }
  EnsureUuids();
  redo_.push_back({songs_, uuids_, current_row_});
  songs_ = undo_.back().songs;
  uuids_ = undo_.back().uuids;
  current_row_ = undo_.back().current_row;
  undo_.pop_back();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Redo() {
  if (redo_.empty()) {
    return;
  }
  EnsureUuids();
  undo_.push_back({songs_, uuids_, current_row_});
  songs_ = redo_.back().songs;
  uuids_ = redo_.back().uuids;
  current_row_ = redo_.back().current_row;
  redo_.pop_back();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::ReplaceSongs(const SongList &songs) {
  MaybeRecordUndo(static_cast<int>(songs.size()));
  songs_ = songs;
  uuids_.clear();
  EnsureUuids();
  if (songs_.empty()) {
    current_row_ = -1;
  } else if (current_row_ < 0 || current_row_ >= static_cast<int>(songs_.size())) {
    current_row_ = 0;
  }
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::InsertSongs(int row, const SongList &songs) {
  if (songs.empty()) {
    return;
  }
  MaybeRecordUndo(static_cast<int>(songs.size()));
  EnsureUuids();
  if (row < 0 || row > static_cast<int>(songs_.size())) {
    row = static_cast<int>(songs_.size());
  }
  songs_.insert(songs_.begin() + row, songs.begin(), songs.end());
  uuids_.insert(uuids_.begin() + row, static_cast<size_t>(songs.size()), {});
  EnsureUuids();
  played_indexes_ = PlaylistPlayed::AfterInsert(played_indexes_, row, static_cast<int>(songs.size()));
  if (current_row_ < 0 && !songs_.empty()) {
    current_row_ = 0;
  }
  MaybeAutoSort();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::AppendSongs(const SongList &songs) { InsertSongs(static_cast<int>(songs_.size()), songs); }

void Playlist::RemoveRows(const std::vector<int> &rows) { RemoveRowsInternal(rows, true); }

void Playlist::RemoveRowsInternal(const std::vector<int> &rows, bool record_undo) {
  if (rows.empty()) {
    return;
  }
  if (record_undo) {
    MaybeRecordUndo(static_cast<int>(rows.size()));
  }
  played_indexes_ = PlaylistPlayed::AfterRemove(played_indexes_, rows);
  std::vector<int> sorted = rows;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  EnsureUuids();
  for (int row : sorted) {
    if (row >= 0 && row < static_cast<int>(songs_.size())) {
      songs_.erase(songs_.begin() + row);
      if (row < static_cast<int>(uuids_.size())) {
        uuids_.erase(uuids_.begin() + row);
      }
      if (current_row_ >= row) {
        current_row_ = std::max(-1, current_row_ - 1);
      }
    }
  }
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Clear() {
  if (songs_.empty()) {
    return;
  }
  MaybeRecordUndo(row_count());
  songs_.clear();
  uuids_.clear();
  current_row_ = -1;
  played_indexes_.clear();
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Move(int from, int to) { MoveRows({from}, to); }

void Playlist::MoveRows(const std::vector<int> &rows, int to) {
  if (rows.empty()) {
    return;
  }
  std::vector<int> sorted = rows;
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  EnsureUuids();
  SongList moving;
  std::vector<std::string> moving_uuids;
  for (int row : sorted) {
    if (row < 0 || row >= row_count()) {
      return;
    }
    moving.push_back(songs_[static_cast<size_t>(row)]);
    moving_uuids.push_back(uuids_[static_cast<size_t>(row)]);
  }
  const Song playing = current_song();
  played_indexes_ = PlaylistPlayed::AfterMove(played_indexes_, row_count(), sorted, to);
  int dest = std::clamp(to, 0, row_count());
  PushUndo();
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
    songs_.erase(songs_.begin() + *it);
    uuids_.erase(uuids_.begin() + *it);
    if (*it < dest) {
      --dest;
    }
  }
  dest = std::clamp(dest, 0, row_count());
  songs_.insert(songs_.begin() + dest, moving.begin(), moving.end());
  uuids_.insert(uuids_.begin() + dest, moving_uuids.begin(), moving_uuids.end());
  current_row_ = -1;
  if (playing.is_valid() || !playing.url().empty()) {
    for (int i = 0; i < row_count(); ++i) {
      if (songs_[static_cast<size_t>(i)] == playing) {
        current_row_ = i;
        break;
      }
    }
  }
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::Shuffle() {
  if (songs_.size() < 2) {
    return;
  }
  PushUndo();
  EnsureUuids();
  std::vector<size_t> order(songs_.size());
  std::iota(order.begin(), order.end(), 0);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(order.begin(), order.end(), gen);
  SongList shuffled_songs;
  std::vector<std::string> shuffled_uuids;
  shuffled_songs.reserve(songs_.size());
  shuffled_uuids.reserve(uuids_.size());
  for (size_t i : order) {
    shuffled_songs.push_back(songs_[i]);
    shuffled_uuids.push_back(uuids_[i]);
  }
  songs_ = std::move(shuffled_songs);
  uuids_ = std::move(shuffled_uuids);
  RebuildVirtualItems();
  Changed.Emit();
}

void Playlist::RemoveDuplicates() {
  PushUndo();
  EnsureUuids();
  SongList unique;
  std::vector<std::string> unique_uuids;
  for (size_t i = 0; i < songs_.size(); ++i) {
    const Song &song = songs_[i];
    const bool exists = std::any_of(unique.begin(), unique.end(), [&](const Song &other) {
      return !song.url().empty() && other.url() == song.url();
    });
    if (!exists) {
      unique.push_back(song);
      unique_uuids.push_back(uuids_[i]);
    }
  }
  songs_ = std::move(unique);
  uuids_ = std::move(unique_uuids);
  if (current_row_ >= static_cast<int>(songs_.size())) {
    current_row_ = songs_.empty() ? -1 : static_cast<int>(songs_.size()) - 1;
  }
  Changed.Emit();
}

void Playlist::InvalidateDeletedSongs(TagReader *tagreader) {
  bool changed = false;
  for (int i = 0; i < row_count(); ++i) {
    Song &song = songs_[static_cast<size_t>(i)];
    if (!PlaylistBehaviour::IsLocalMedia(song)) {
      continue;
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    const bool exists = !path.empty() && FileUtils::Exists(path);
    const Song before = song;
    if (PlaylistBehaviour::ApplyLocalExistence(&song, exists)) {
      changed = true;
    }
    if (tagreader && PlaylistReloadRows::ShouldReload(before, exists)) {
      ReloadRow(i, tagreader);
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

bool Playlist::ApplyValidityOnCurrentSong(const std::string &url, bool valid) {
  if (current_row_ < 0 || current_row_ >= row_count()) {
    return false;
  }
  Song &song = songs_[static_cast<size_t>(current_row_)];
  if (!url.empty() && song.url() != url && song.stream_url() != url) {
    return false;
  }
  if (PlaylistBehaviour::ApplyValidity(&song, valid)) {
    Changed.Emit();
  }
  return true;
}

void Playlist::SetSort(PlaylistColumn column, bool descending) {
  sort_column_ = column;
  sort_descending_ = descending;
}

void Playlist::SortNow() {
  if (sort_column_ == PlaylistColumn::Count) {
    return;
  }
  PushUndo();
  SortInPlace();
  Changed.Emit();
}

void Playlist::MaybeAutoSort() {
  if (PlaylistAutoSort::ShouldSort(auto_sort_, loading_, sort_column_)) {
    SortInPlace();
  }
}

void Playlist::SortInPlace() {
  if (sort_column_ == PlaylistColumn::Count || songs_.size() < 2) {
    return;
  }
  EnsureUuids();
  const Song playing = current_song();
  const bool numeric = PlaylistBehaviour::ColumnIsNumeric(sort_column_);
  const PlaylistColumn column = sort_column_;
  const bool descending = sort_descending_;
  std::vector<size_t> order(songs_.size());
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(), [this, column, numeric, descending](size_t a, size_t b) {
    return PlaylistBehaviour::LessThanText(PlaylistDelegates::ColumnText(songs_[a], column), PlaylistDelegates::ColumnText(songs_[b], column),
                                           numeric, descending);
  });
  SongList sorted_songs;
  std::vector<std::string> sorted_uuids;
  sorted_songs.reserve(songs_.size());
  sorted_uuids.reserve(uuids_.size());
  for (size_t i : order) {
    sorted_songs.push_back(songs_[i]);
    sorted_uuids.push_back(uuids_[i]);
  }
  songs_ = std::move(sorted_songs);
  uuids_ = std::move(sorted_uuids);
  current_row_ = -1;
  if (playing.is_valid() || !playing.url().empty()) {
    for (int i = 0; i < row_count(); ++i) {
      if (songs_[static_cast<size_t>(i)] == playing) {
        current_row_ = i;
        break;
      }
    }
  }
}

void Playlist::RemoveUnavailable() {
  PushUndo();
  EnsureUuids();
  SongList keep;
  std::vector<std::string> keep_uuids;
  for (size_t i = 0; i < songs_.size(); ++i) {
    const Song &song = songs_[i];
    const std::string &url = song.url();
    bool drop = false;
    if (!url.empty()) {
      const auto scheme = url.find("://");
      if (scheme == std::string::npos || url.rfind("file://", 0) == 0) {
        const std::string path = FileUtils::PathFromUri(url);
        drop = !path.empty() && !FileUtils::Exists(path);
      }
    }
    if (!drop) {
      keep.push_back(song);
      keep_uuids.push_back(uuids_[i]);
    }
  }
  songs_ = std::move(keep);
  uuids_ = std::move(keep_uuids);
  if (current_row_ >= static_cast<int>(songs_.size())) {
    current_row_ = songs_.empty() ? -1 : static_cast<int>(songs_.size()) - 1;
  }
  Changed.Emit();
}

void Playlist::RenumberTracks() {
  PushUndo();
  for (size_t i = 0; i < songs_.size(); ++i) {
    songs_[i].set_track(static_cast<int>(i) + 1);
  }
  Changed.Emit();
}

void Playlist::RateCurrentSong(float rating) {
  if (current_row_ < 0 || current_row_ >= static_cast<int>(songs_.size())) {
    return;
  }
  PushUndo();
  songs_[static_cast<size_t>(current_row_)].set_rating(rating);
  Changed.Emit();
}

void Playlist::SkipTracks(const std::vector<int> &rows) {
  bool all_skipped = true;
  for (int row : rows) {
    if (row >= 0 && row < row_count() && !songs_[static_cast<size_t>(row)].skipped()) {
      all_skipped = false;
      break;
    }
  }
  PushUndo();
  for (int row : rows) {
    if (row >= 0 && row < row_count()) {
      songs_[static_cast<size_t>(row)].set_skipped(!all_skipped);
    }
  }
  Changed.Emit();
}

void Playlist::ReplaceRow(int row, const Song &song) {
  if (row < 0 || row >= row_count()) {
    return;
  }
  PushUndo();
  const bool skipped = songs_[static_cast<size_t>(row)].skipped();
  songs_[static_cast<size_t>(row)] = song;
  songs_[static_cast<size_t>(row)].set_skipped(skipped);
  Changed.Emit();
}

void Playlist::SetFilterString(const std::string &filter) {
  filter_string_ = filter;
  filter_.SetFilterString(filter);
}

bool Playlist::UpdateRowMetadata(int row, const Song &engine) {
  if (row < 0 || row >= row_count()) {
    return false;
  }
  Song &existing = songs_[static_cast<size_t>(row)];
  const Song before = existing;
  PlayerMetadataSync::Merge(&existing, engine);
  if (PlayerMetadataSync::ShouldRefreshPlaylist(before, existing)) {
    Changed.Emit();
    return true;
  }
  return false;
}

bool Playlist::MergeFromEngine(const Song &engine) {
  if (engine.url().empty()) {
    return false;
  }
  bool changed = false;
  for (Song &existing : songs_) {
    if (existing.url() != engine.url()) {
      continue;
    }
    const Song before = existing;
    PlayerMetadataSync::Merge(&existing, engine);
    if (PlayerMetadataSync::ShouldRefreshPlaylist(before, existing)) {
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
  return changed;
}

void Playlist::UpdateSongsByUrl(const Song &song) {
  if (song.url().empty()) {
    return;
  }
  bool changed = false;
  for (int i = 0; i < row_count(); ++i) {
    if (songs_[static_cast<size_t>(i)].url() == song.url()) {
      const bool skipped = songs_[static_cast<size_t>(i)].skipped();
      songs_[static_cast<size_t>(i)] = song;
      songs_[static_cast<size_t>(i)].set_skipped(skipped);
      changed = true;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

void Playlist::UpdateItems(const SongList &songs) {
  bool changed = false;
  for (int i = 0; i < row_count(); ++i) {
    Song &existing = songs_[static_cast<size_t>(i)];
    for (const Song &incoming : songs) {
      if (!PlaylistMetadataUpdate::ShouldReplace(existing, incoming)) {
        continue;
      }
      const bool skipped = existing.skipped();
      existing = incoming;
      existing.set_skipped(skipped);
      changed = true;
      break;
    }
  }
  if (changed) {
    Changed.Emit();
  }
}

bool Playlist::SetColumnValue(int row, PlaylistColumn column, const std::string &value) {
  return SetColumnValues({row}, column, value) > 0;
}

int Playlist::SetColumnValues(const std::vector<int> &rows, PlaylistColumn column, const std::string &value) {
  if (!PlaylistDelegates::ColumnIsEditable(column)) {
    return 0;
  }
  PushUndo();
  int updated = 0;
  for (int row : rows) {
    if (row < 0 || row >= row_count()) {
      continue;
    }
    Song song = songs_[static_cast<size_t>(row)];
    if (!PlaylistDelegates::SetColumnValue(song, column, value)) {
      continue;
    }
    const bool skipped = songs_[static_cast<size_t>(row)].skipped();
    songs_[static_cast<size_t>(row)] = song;
    songs_[static_cast<size_t>(row)].set_skipped(skipped);
    ++updated;
  }
  if (updated == 0) {
    undo_.pop_back();
    return 0;
  }
  Changed.Emit();
  return updated;
}

unsigned long long Playlist::SaveGeneration(const std::string &uuid) const {
  const auto it = save_generations_.find(uuid);
  return it == save_generations_.end() ? 0 : it->second;
}

unsigned long long Playlist::BumpSaveGeneration(const std::string &uuid) {
  if (uuid.empty()) {
    return 0;
  }
  return save_generations_[uuid] = PlaylistSaveItem::Begin(SaveGeneration(uuid));
}

int Playlist::RowForUuid(const std::string &uuid) const {
  if (uuid.empty()) {
    return -1;
  }
  for (int i = 0; i < row_count(); ++i) {
    if (UuidAt(i) == uuid) {
      return i;
    }
  }
  return -1;
}

void Playlist::SaveRows(const std::vector<int> &rows) {
  if (!tagreader_client_ || rows.empty()) {
    return;
  }
  EnsureUuids();
  bool queued = false;
  for (int row : rows) {
    if (row < 0 || row >= row_count()) {
      continue;
    }
    const Song song = songs_[static_cast<size_t>(row)];
    if (!PlaylistSaveItem::ShouldWriteFile(song)) {
      continue;
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    if (path.empty()) {
      continue;
    }
    const std::string uuid = UuidAt(row);
    const unsigned long long generation = BumpSaveGeneration(uuid);
    TagReaderReplyPtr reply = tagreader_client_->WriteFileAsync(path, song);
    auto alive = std::make_shared<bool>(true);
    pending_save_flags_.push_back(alive);
    reply->Finished.Connect([this, alive, uuid, generation, song, reply](const std::string &, const TagReaderResult &) {
      if (!alive || !*alive) {
        return;
      }
      SaveRowComplete(uuid, generation, song, reply);
    });
    queued = true;
  }
  if (queued) {
    SaveQueued.Emit();
  }
}

void Playlist::SaveRowComplete(const std::string &uuid, unsigned long long generation, const Song &pre_edit, TagReaderReplyPtr reply) {
  if (!PlaylistSaveItem::ShouldApply(generation, SaveGeneration(uuid))) {
    return;
  }
  if (!reply || !reply->success()) {
    Error.Emit(PlaylistSaveItem::WriteError(reply ? reply->filename() : std::string{}, reply ? reply->error() : std::string{}));
    ReloadSavedRow(uuid, generation, pre_edit);
    return;
  }
  ReloadSavedRow(uuid, generation, Song());
}

void Playlist::ReloadSavedRow(const std::string &uuid, unsigned long long generation, const Song &fallback) {
  if (!tagreader_client_ || !PlaylistSaveItem::ShouldApply(generation, SaveGeneration(uuid))) {
    return;
  }
  const int row = RowForUuid(uuid);
  if (row < 0) {
    return;
  }
  const std::string path = FileUtils::PathFromUri(songs_[static_cast<size_t>(row)].url());
  if (path.empty()) {
    ApplyReloadedRow(uuid, generation, Song(), fallback, false);
    return;
  }
  TagReaderReadFileReplyPtr reply = tagreader_client_->ReadFileAsync(path);
  auto alive = std::make_shared<bool>(true);
  pending_save_flags_.push_back(alive);
  reply->SongFinished.Connect([this, alive, uuid, generation, fallback, reply](const std::string &, const Song &, const TagReaderResult &) {
    if (!alive || !*alive) {
      return;
    }
    ApplyReloadedRow(uuid, generation, reply->song(), fallback, reply->success());
  });
  SaveQueued.Emit();
}

void Playlist::ApplyReloadedRow(const std::string &uuid, unsigned long long generation, const Song &from_file, const Song &fallback,
                               bool read_ok) {
  if (!PlaylistSaveItem::ShouldApply(generation, SaveGeneration(uuid))) {
    return;
  }
  const int row = RowForUuid(uuid);
  if (row < 0) {
    return;
  }
  Song applied = PlaylistSaveItem::ChooseMetadata(read_ok, from_file, fallback);
  if (!applied.is_valid()) {
    return;
  }
  const Song current = songs_[static_cast<size_t>(row)];
  if (current.id() > 0) {
    applied.set_id(current.id());
  }
  applied.set_skipped(current.skipped());
  if (!applied.url().empty() && current.url() != applied.url() && !current.url().empty()) {
    applied.set_url(current.url());
  }
  songs_[static_cast<size_t>(row)] = applied;
  Changed.Emit();
  ItemSaved.Emit(applied);
}

void Playlist::ReloadRow(int row, TagReader *tagreader) {
  if (!tagreader || row < 0 || row >= row_count()) {
    return;
  }
  const std::string path = FileUtils::PathFromUri(songs_[static_cast<size_t>(row)].url());
  if (path.empty() || !FileUtils::Exists(path)) {
    return;
  }
  Song updated = tagreader->ReadFile(path);
  if (!updated.is_valid()) {
    return;
  }
  updated.set_id(songs_[static_cast<size_t>(row)].id());
  updated.set_skipped(songs_[static_cast<size_t>(row)].skipped());
  PushUndo();
  songs_[static_cast<size_t>(row)] = updated;
  Changed.Emit();
}

void Playlist::SetSequenceMode(SequenceMode mode) {
  mode_ = mode;
  switch (mode) {
    case SequenceMode::RepeatTrack:
      repeat_mode_ = PlaylistSequence::RepeatMode::Track;
      break;
    case SequenceMode::RepeatAll:
      repeat_mode_ = PlaylistSequence::RepeatMode::Playlist;
      break;
    case SequenceMode::Shuffle:
      shuffle_mode_ = PlaylistSequence::ShuffleMode::All;
      break;
    case SequenceMode::AlbumShuffle:
      shuffle_mode_ = PlaylistSequence::ShuffleMode::InsideAlbum;
      break;
    case SequenceMode::Sequential:
      repeat_mode_ = PlaylistSequence::RepeatMode::Off;
      break;
    case SequenceMode::Dynamic:
      break;
  }
}

void Playlist::SetRepeatMode(PlaylistSequence::RepeatMode mode) {
  repeat_mode_ = mode;
  if (mode == PlaylistSequence::RepeatMode::Track) {
    mode_ = SequenceMode::RepeatTrack;
  } else if (mode == PlaylistSequence::RepeatMode::Playlist) {
    mode_ = SequenceMode::RepeatAll;
  } else if (mode == PlaylistSequence::RepeatMode::Off) {
    mode_ = SequenceMode::Sequential;
  }
  RepeatModeChanged.Emit();
}

void Playlist::SetShuffleMode(PlaylistSequence::ShuffleMode mode) {
  shuffle_mode_ = mode;
  if (mode == PlaylistSequence::ShuffleMode::All) {
    mode_ = SequenceMode::Shuffle;
  } else if (mode == PlaylistSequence::ShuffleMode::InsideAlbum) {
    mode_ = SequenceMode::AlbumShuffle;
  }
  RebuildVirtualItems();
  ShuffleModeChanged.Emit();
}

void Playlist::Reshuffle(unsigned seed) { RebuildVirtualItems(seed == 0 ? std::random_device{}() : seed); }

void Playlist::SyncVirtualIndex() {
  current_virtual_index_ = -1;
  for (int i = 0; i < static_cast<int>(virtual_items_.size()); ++i) {
    if (virtual_items_[static_cast<size_t>(i)] == current_row_) {
      current_virtual_index_ = i;
      return;
    }
  }
}

bool Playlist::SameAlbum(int left, int right) const {
  return PlaylistShuffle::AlbumKey(song(left)) == PlaylistShuffle::AlbumKey(song(right));
}

void Playlist::RebuildVirtualItems(unsigned seed) {
  const int n = row_count();
  if (seed == 0 && shuffle_mode_ != PlaylistSequence::ShuffleMode::Off) {
    seed = std::random_device{}();
  }
  std::vector<std::string> keys;
  keys.reserve(static_cast<size_t>(n));
  switch (shuffle_mode_) {
    case PlaylistSequence::ShuffleMode::All:
    case PlaylistSequence::ShuffleMode::InsideAlbum:
      virtual_items_ = PlaylistShuffle::ShuffleAll(n, seed, current_row_);
      break;
    case PlaylistSequence::ShuffleMode::Albums:
      for (int i = 0; i < n; ++i) {
        keys.push_back(PlaylistShuffle::AlbumKey(song(i)));
      }
      virtual_items_ = PlaylistShuffle::ShuffleByKey(keys, seed, current_row_);
      break;
    case PlaylistSequence::ShuffleMode::Grouping:
      for (int i = 0; i < n; ++i) {
        keys.push_back(PlaylistShuffle::GroupingKey(song(i)));
      }
      virtual_items_ = PlaylistShuffle::ShuffleByKey(keys, seed, current_row_);
      break;
    case PlaylistSequence::ShuffleMode::Off:
    default:
      virtual_items_ = PlaylistShuffle::Identity(n);
      break;
  }
  SyncVirtualIndex();
}

int Playlist::NextIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  if (mode_ == SequenceMode::RepeatTrack || repeat_mode_ == PlaylistSequence::RepeatMode::Track) {
    return PlaylistFilterIndex::RepeatTrackRow(current_row_, filter_, current_song());
  }
  if (static_cast<int>(virtual_items_.size()) != row_count()) {
    const_cast<Playlist *>(this)->RebuildVirtualItems();
  }
  const bool wrap = mode_ == SequenceMode::RepeatAll || repeat_mode_ == PlaylistSequence::RepeatMode::Playlist;
  const bool album_only = repeat_mode_ == PlaylistSequence::RepeatMode::Album ||
                          shuffle_mode_ == PlaylistSequence::ShuffleMode::InsideAlbum;
  const int n = static_cast<int>(virtual_items_.size());
  const int start = current_virtual_index_ >= 0 ? current_virtual_index_ : -1;
  auto accept = [&](int virt) -> int {
    if (virt < 0 || virt >= n) {
      return -1;
    }
    const int row = virtual_items_[static_cast<size_t>(virt)];
    const Song candidate = song(row);
    if (candidate.skipped()) {
      return -1;
    }
    if (!filter_.Accepts(candidate)) {
      return -1;
    }
    if (album_only && current_row_ >= 0 && !SameAlbum(row, current_row_)) {
      return -1;
    }
    return row;
  };
  for (int i = start + 1; i < n; ++i) {
    const int row = accept(i);
    if (row >= 0) {
      return row;
    }
  }
  if (wrap || album_only) {
    for (int i = 0; i < n; ++i) {
      const int row = accept(i);
      if (row >= 0 && row != current_row_) {
        return row;
      }
    }
    return current_row_;
  }
  return -1;
}

int Playlist::PreviousIndex() const {
  if (songs_.empty()) {
    return -1;
  }
  if (static_cast<int>(virtual_items_.size()) != row_count()) {
    const_cast<Playlist *>(this)->RebuildVirtualItems();
  }
  const bool wrap = mode_ == SequenceMode::RepeatAll || repeat_mode_ == PlaylistSequence::RepeatMode::Playlist;
  const bool album_only = repeat_mode_ == PlaylistSequence::RepeatMode::Album ||
                          shuffle_mode_ == PlaylistSequence::ShuffleMode::InsideAlbum;
  const int n = static_cast<int>(virtual_items_.size());
  const int start = current_virtual_index_ >= 0 ? current_virtual_index_ : 0;
  auto accept = [&](int virt) -> int {
    if (virt < 0 || virt >= n) {
      return -1;
    }
    const int row = virtual_items_[static_cast<size_t>(virt)];
    const Song candidate = song(row);
    if (candidate.skipped()) {
      return -1;
    }
    if (!filter_.Accepts(candidate)) {
      return -1;
    }
    if (album_only && current_row_ >= 0 && !SameAlbum(row, current_row_)) {
      return -1;
    }
    return row;
  };
  for (int i = start - 1; i >= 0; --i) {
    const int row = accept(i);
    if (row >= 0) {
      return row;
    }
  }
  if (!wrap) {
    return current_row_;
  }
  for (int i = n - 1; i >= 0; --i) {
    const int row = accept(i);
    if (row >= 0) {
      return row;
    }
  }
  return current_row_;
}

void Playlist::Next() {
  const int old = current_row_;
  const int next = NextIndex();
  if (next >= 0) {
    PlaylistPlayed::Push(&played_indexes_, current_row_);
    set_current_row(next);
    MaintainDynamicAfterAdvance(old);
  }
}

void Playlist::Previous() {
  const int played = PlaylistPlayed::Pop(&played_indexes_);
  if (played >= 0 && played < row_count()) {
    set_current_row(played);
    return;
  }
  const int previous = PreviousIndex();
  if (previous >= 0) {
    set_current_row(previous);
  }
}

void Playlist::RecordAndSetCurrentRow(int row) {
  const int old = current_row_;
  if (current_row_ >= 0 && current_row_ != row) {
    PlaylistPlayed::Push(&played_indexes_, current_row_);
  }
  set_current_row(row);
  if (row > old) {
    MaintainDynamicAfterAdvance(old);
  }
}

void Playlist::MaintainDynamicAfterAdvance(int old_row) {
  if (!PlaylistDynamicAdvance::ShouldReplenish(dynamic_, current_row_ > old_row && old_row >= 0)) {
    return;
  }
  undo_.clear();
  redo_.clear();
  const int max_history = PlaylistDynamicAdvance::MaxHistory(dynamic_generator_.get());
  const int max_future = PlaylistDynamicAdvance::MaxFuture(dynamic_generator_.get());
  const int trim = DynamicPlaylistMaintenance::HistoryTrimCount(DynamicPlaylistMaintenance::HistoryLength(current_row_), max_history);
  if (trim > 0) {
    std::vector<int> rows(static_cast<size_t>(trim));
    std::iota(rows.begin(), rows.end(), 0);
    RemoveRowsInternal(rows, false);
  }
  const int want = PlaylistDynamicAdvance::ReplenishCount(DynamicPlaylistMaintenance::HistoryLength(current_row_), max_future, row_count());
  if (want > 0) {
    InsertDynamicMore(want);
  }
}

void Playlist::InsertDynamicMore(int count) {
  if (count <= 0) {
    return;
  }
  if (dynamic_generator_) {
    if (auto *query = dynamic_cast<PlaylistQueryGenerator *>(dynamic_generator_.get())) {
      query->Remember(songs_);
    }
    SongList extra = PlaylistDynamicAdvance::DedupByIdThenUrl(dynamic_generator_->GenerateMore(count), songs_);
    if (!extra.empty()) {
      AppendSongs(extra);
    }
    return;
  }
}

int64_t Playlist::total_length_nanosec() const {
  return std::accumulate(songs_.begin(), songs_.end(), int64_t{0},
                         [](int64_t total, const Song &song) { return total + std::max<int64_t>(0, song.length_nanosec()); });
}

void Playlist::SetDynamic(bool dynamic, const SmartPlaylistSearch &search) {
  dynamic_ = dynamic;
  dynamic_search_ = search;
  if (dynamic) {
    mode_ = SequenceMode::Dynamic;
    CollectionBackend *backend = dynamic_generator_ ? dynamic_generator_->collection() : nullptr;
    auto query = std::make_shared<PlaylistQueryGenerator>(name_, search, true);
    query->set_collection_backend(backend);
    query->Remember(songs_);
    dynamic_generator_ = std::move(query);
  } else {
    if (mode_ == SequenceMode::Dynamic) {
      mode_ = SequenceMode::Sequential;
    }
    dynamic_generator_.reset();
  }
}

void Playlist::SetDynamicGenerator(std::shared_ptr<PlaylistGenerator> generator) {
  dynamic_generator_ = std::move(generator);
  dynamic_ = static_cast<bool>(dynamic_generator_);
  if (dynamic_generator_) {
    mode_ = SequenceMode::Dynamic;
    dynamic_generator_->set_dynamic(true);
    if (auto *query = dynamic_cast<PlaylistQueryGenerator *>(dynamic_generator_.get())) {
      dynamic_search_ = query->search();
      query->Remember(songs_);
    }
  } else if (mode_ == SequenceMode::Dynamic) {
    mode_ = SequenceMode::Sequential;
  }
}

void Playlist::ApplyDiscoveredArt(const Song &playing, const std::string &discovered) {
  if (current_row_ < 0 || current_row_ >= row_count()) {
    return;
  }
  Song row = current_song();
  if (!PlaylistLocalArtDiscover::ShouldWriteSidecar(row, playing, discovered)) {
    return;
  }
  PlaylistLocalArtDiscover::ApplySidecar(&row, discovered);
  ReplaceRow(current_row_, row);
}

void Playlist::RefillDynamic(const SongList &pool, bool force) {
  if (!dynamic_) {
    return;
  }
  const int upcoming = DynamicPlaylistMaintenance::FutureCount(row_count(), current_row_);
  const int max_future = PlaylistDynamicAdvance::MaxFuture(dynamic_generator_.get());
  if (!force && upcoming >= max_future) {
    return;
  }
  const int want = force ? max_future : std::max(0, max_future - upcoming);
  if (want <= 0) {
    return;
  }
  if (dynamic_generator_ && dynamic_generator_->collection()) {
    InsertDynamicMore(want);
    return;
  }
  SongList extra = PlaylistDynamicAdvance::DedupByIdThenUrl(dynamic_search_.Search(pool), songs_);
  if (static_cast<int>(extra.size()) > want) {
    extra.resize(static_cast<size_t>(want));
  }
  if (!extra.empty()) {
    AppendSongs(extra);
  }
}

void Playlist::ExpandDynamic(const SongList &pool) { RefillDynamic(pool, true); }

void Playlist::RepopulateDynamic(const SongList &pool) {
  if (!dynamic_) {
    return;
  }
  RemoveItemsNotInQueue();
  RefillDynamic(pool, true);
}

void Playlist::RemoveItemsNotInQueue() {
  const std::vector<int> rows = PlaylistRemoveItemsNotInQueue::RowsToRemove(row_count(), current_row_, [this](int row) {
    return queue_.ContainsPlaylistRow(id_, row);
  });
  if (!rows.empty()) {
    RemoveRows(rows);
  }
}
