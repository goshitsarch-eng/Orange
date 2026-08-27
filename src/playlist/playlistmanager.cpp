#include "playlist/playlistmanager.h"

#include "collection/collectionbackend.h"
#include "playlist/playlistcollectionsync.h"
#include "playlist/playlistcrossundopair.h"
#include "playlist/playlistrating.h"
#include "playlist/playlistsaveschedule.h"
#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "core/songloader.h"
#include "playlist/songloaderinserter.h"
#include "playlistparsers/playlistparser.h"
#include "smartplaylists/playlistgeneratorinserter.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "smartplaylists/smartplaylistsmodel.h"
#include "tagreader/tagreader.h"
#include "tagreader/tagreaderclient.h"
#include "tagreader/tagreaderclientpump.h"
#include "utilities/fileutils.h"

#include <algorithm>

PlaylistManager::PlaylistManager(TaskManager *task_manager, TagReader *tagreader, UrlHandlers *url_handlers, PlaylistBackend *backend,
                                 CollectionBackend *collection_backend)
    : task_manager_(task_manager),
      tagreader_(tagreader),
      url_handlers_(url_handlers),
      backend_(backend),
      collection_backend_(collection_backend) {}

namespace {

gboolean PlaylistManagerTagPumpCb(gpointer data) {
  auto *self = static_cast<PlaylistManager *>(data);
  return self->PumpTagReader() ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

}  // namespace

PlaylistManager::~PlaylistManager() {
  if (tag_pump_id_) {
    g_source_remove(tag_pump_id_);
    tag_pump_id_ = 0;
  }
  FlushPendingSaves();
}

void PlaylistManager::Init() {
  LoadAll();
  if (collection_backend_) {
    collection_backend_->SongsChanged.Connect([this](const SongList &songs) { UpdateCollectionSongs(songs); });
    collection_backend_->SongsStatisticsChanged.Connect([this](const SongList &songs) { UpdateCollectionSongs(songs); });
    collection_backend_->SongsRatingChanged.Connect([this](const SongList &songs) { UpdateCollectionSongs(songs); });
  }
}

void PlaylistManager::UpdateCollectionSongs(const SongList &songs) { PlaylistCollectionSync::PatchAll(GetAllPlaylists(), songs); }

void PlaylistManager::set_tagreader_client(TagReaderClient *client) {
  tagreader_client_ = client;
  for (const auto &playlist : playlists_) {
    playlist->set_tagreader_client(client);
  }
}

void PlaylistManager::WatchSaves(Playlist *playlist) {
  if (!playlist) {
    return;
  }
  playlist->set_tagreader_client(tagreader_client_);
  playlist->SaveQueued.Connect([this]() { ArmTagReaderPump(); });
  playlist->ItemSaved.Connect([this](const Song &song) {
    if (collection_backend_ && (song.id() > 0 || song.is_collection_song())) {
      collection_backend_->AddOrUpdateSong(song);
    }
  });
  playlist->Error.Connect([this](const std::string &message) { Error.Emit(message); });
  playlist->Changed.Connect([this, playlist]() { PlaylistChanged.Emit(playlist); });
}

void PlaylistManager::ArmTagReaderPump() {
  if (!TagReaderClientPump::ShouldArm(tagreader_client_ && tagreader_client_->HaveRequests(), tag_pump_id_ != 0)) {
    return;
  }
  tag_pump_id_ = g_idle_add(PlaylistManagerTagPumpCb, this);
}

bool PlaylistManager::PumpTagReader() {
  if (!tagreader_client_ || !tagreader_client_->ProcessNext()) {
    tag_pump_id_ = 0;
    return false;
  }
  return TagReaderClientPump::ShouldContinue(tagreader_client_->HaveRequests());
}

void PlaylistManager::LoadAll() {
  playlists_.clear();
  current_ = nullptr;
  active_ = nullptr;
  if (backend_) {
    for (const PlaylistMetadata &metadata : backend_->GetAllPlaylists()) {
      playlists_.push_back(backend_->LoadPlaylist(metadata.id));
    }
  }
  if (playlists_.empty()) {
    New("Playlist");
  } else {
    current_ = playlists_.front().get();
    active_ = current_;
    for (const auto &playlist : playlists_) {
      WatchSaves(playlist.get());
      if (playlist->id() >= next_id_) {
        next_id_ = playlist->id() + 1;
      }
    }
    Settings settings;
    settings.BeginGroup(PlaylistSettings::kSettingsGroup);
    if (settings.BoolValue(PlaylistSettings::kGreyoutSongsStartup, PlaylistSettings::kDefaultGreyoutSongsStartup)) {
      for (const auto &playlist : playlists_) {
        playlist->InvalidateDeletedSongs(tagreader_);
      }
    }
  }
  playlists_loaded_ = true;
  PlaylistsLoaded.Emit();
}

int PlaylistManager::current_id() const { return current_ ? current_->id() : -1; }

int PlaylistManager::active_id() const { return active_ ? active_->id() : -1; }

std::vector<int> PlaylistManager::playlist_ids() const {
  std::vector<int> ids;
  ids.reserve(playlists_.size());
  for (const auto &playlist : playlists_) {
    ids.push_back(playlist->id());
  }
  return ids;
}

std::string PlaylistManager::playlist_name(int id) const {
  if (const Playlist *found = FindById(id)) {
    return found->name();
  }
  return {};
}

Playlist *PlaylistManager::playlist(int id) const { return FindById(id); }

std::vector<Playlist *> PlaylistManager::GetAllPlaylists() const {
  std::vector<Playlist *> result;
  result.reserve(playlists_.size());
  for (const auto &playlist : playlists_) {
    result.push_back(playlist.get());
  }
  return result;
}

std::vector<std::string> PlaylistManager::playlist_names() const {
  std::vector<std::string> names;
  names.reserve(playlists_.size());
  for (const auto &playlist : playlists_) {
    names.push_back(playlist->name());
  }
  return names;
}

void PlaylistManager::RemoveDeletedSongs() {
  for (const auto &playlist : playlists_) {
    playlist->RemoveUnavailable();
    Persist(playlist.get());
  }
}

Playlist *PlaylistManager::New(const std::string &name, const SongList &songs) {
  auto playlist = std::make_unique<Playlist>();
  playlist->set_name(name);
  if (!songs.empty()) {
    playlist->AppendSongs(songs);
  }
  PersistNow(playlist.get());
  if (playlist->id() < 0) {
    playlist->set_id(next_id_++);
  } else if (playlist->id() >= next_id_) {
    next_id_ = playlist->id() + 1;
  }
  current_ = playlist.get();
  if (!active_) {
    active_ = current_;
  }
  playlists_.push_back(std::move(playlist));
  WatchSaves(current_);
  PlaylistAdded.Emit(current_);
  CurrentChanged.Emit(current_);
  return current_;
}

void PlaylistManager::Load(const std::string &filename) {
  const SongList songs = PlaylistParser(collection_backend_).Load(filename);
  std::string name = FileUtils::BaseName(filename);
  const auto dot = name.rfind('.');
  if (dot != std::string::npos && dot > 0) {
    name = name.substr(0, dot);
  }
  if (name.empty()) {
    name = "Playlist";
  }
  New(name, songs);
}

void PlaylistManager::Save(int id, const std::string &filename) {
  if (Playlist *found = FindById(id)) {
    PlaylistParser().Save(filename, found->songs());
  }
}

void PlaylistManager::Rename(int id, const std::string &new_name) {
  Playlist *found = FindById(id);
  if (!found || new_name.empty()) {
    return;
  }
  found->set_name(new_name);
  if (backend_) {
    backend_->RenamePlaylist(id, new_name);
  }
  PlaylistRenamed.Emit(id, new_name);
  PlaylistChanged.Emit(found);
}

void PlaylistManager::Favorite(int id, bool favorite) {
  if (Playlist *found = FindById(id)) {
    found->set_favorite(favorite);
    if (backend_) {
      backend_->SetFavorite(id, favorite);
    }
    PlaylistFavorited.Emit(id, favorite);
  }
}

void PlaylistManager::SetPlaylistUiPath(int id, const std::string &path) {
  if (Playlist *found = FindById(id)) {
    found->set_ui_path(path);
  }
  if (backend_) {
    backend_->SetPlaylistUiPath(id, path);
  }
}

void PlaylistManager::Delete(int id) {
  if (Playlist *found = FindById(id)) {
    found->set_favorite(false);
  }
  if (!Close(id)) {
    if (backend_) {
      backend_->DeletePlaylist(id);
    }
    PlaylistDeleted.Emit(id);
  }
}

bool PlaylistManager::Close(int id) {
  FlushPendingSaves();
  Playlist *found = FindById(id);
  if (!found) {
    return false;
  }
  const bool favorite = found->favorite();
  playlists_.erase(std::remove_if(playlists_.begin(), playlists_.end(),
                                  [id](const std::unique_ptr<Playlist> &playlist) { return playlist->id() == id; }),
                   playlists_.end());
  if (current_ && current_->id() == id) {
    current_ = playlists_.empty() ? nullptr : playlists_.front().get();
  }
  if (active_ && active_->id() == id) {
    active_ = current_;
  }
  if (playlists_.empty()) {
    New("Playlist");
  } else if (current_) {
    CurrentChanged.Emit(current_);
  }
  PlaylistClosed.Emit(id);
  if (!favorite && backend_) {
    backend_->DeletePlaylist(id);
    PlaylistDeleted.Emit(id);
  }
  return true;
}

void PlaylistManager::Open(int id) {
  if (Playlist *found = FindById(id)) {
    SetCurrentPlaylist(id);
    (void)found;
    return;
  }
  if (!backend_) {
    return;
  }
  auto playlist = backend_->LoadPlaylist(id);
  if (!playlist) {
    return;
  }
  current_ = playlist.get();
  if (current_->id() >= next_id_) {
    next_id_ = current_->id() + 1;
  }
  playlists_.push_back(std::move(playlist));
  WatchSaves(current_);
  PlaylistAdded.Emit(current_);
  CurrentChanged.Emit(current_);
}

void PlaylistManager::ChangePlaylistOrder(const std::vector<int> &ids) {
  std::vector<std::unique_ptr<Playlist>> reordered;
  reordered.reserve(playlists_.size());
  for (int id : ids) {
    auto it = std::find_if(playlists_.begin(), playlists_.end(),
                           [id](const std::unique_ptr<Playlist> &playlist) { return playlist && playlist->id() == id; });
    if (it != playlists_.end()) {
      reordered.push_back(std::move(*it));
      playlists_.erase(it);
    }
  }
  for (auto &playlist : playlists_) {
    if (playlist) {
      reordered.push_back(std::move(playlist));
    }
  }
  playlists_ = std::move(reordered);
}

void PlaylistManager::SetCurrentPlaylist(const std::string &name) {
  if (Playlist *found = FindByName(name)) {
    current_ = found;
    CurrentChanged.Emit(current_);
  }
}

void PlaylistManager::SetCurrentPlaylist(int id) {
  if (Playlist *found = FindById(id)) {
    current_ = found;
    CurrentChanged.Emit(current_);
  }
}

void PlaylistManager::SetActivePlaylist(int id) {
  if (Playlist *found = FindById(id)) {
    active_ = found;
    ActiveChanged.Emit(active_);
  }
}

void PlaylistManager::SetActiveToCurrent() {
  if (current_ && active_ != current_) {
    active_ = current_;
    ActiveChanged.Emit(active_);
  }
}

void PlaylistManager::SetCurrentRow(int row) {
  if (Playlist *playlist = Visible()) {
    playlist->RecordAndSetCurrentRow(row);
    PersistLastPlayed(playlist);
    if (active_ != playlist) {
      active_ = playlist;
      ActiveChanged.Emit(active_);
    }
  }
}

int PlaylistManager::current_row() const {
  if (const Playlist *playlist = Playing()) {
    return playlist->current_row();
  }
  return -1;
}

Song PlaylistManager::current_song() const { return Playing() ? Playing()->current_song() : Song(); }

Song PlaylistManager::PeekNextSong() const { return Playing() ? Playing()->PeekNextSong() : Song(); }

void PlaylistManager::Next() {
  if (Playlist *playlist = Playing()) {
    playlist->Next();
    PersistLastPlayed(playlist);
    RefillDynamic();
  }
}

void PlaylistManager::Previous() {
  if (Playlist *playlist = Playing()) {
    playlist->Previous();
    PersistLastPlayed(playlist);
  }
}

void PlaylistManager::AppendSongs(const SongList &songs) {
  if (Playlist *playlist = Visible()) {
    playlist->AppendSongs(songs);
    Persist(playlist);
  }
}

void PlaylistManager::InsertSongs(int id, const SongList &songs, int pos) {
  Playlist *found = FindById(id);
  if (!found) {
    return;
  }
  if (pos < 0) {
    found->AppendSongs(songs);
  } else {
    found->InsertSongs(pos, songs);
  }
  Persist(found);
}

void PlaylistManager::MoveRowsBetween(int source_id, int dest_id, const std::vector<int> &rows, int dest_pos) {
  Playlist *source = FindById(source_id);
  Playlist *dest = FindById(dest_id);
  if (!source || !dest || source == dest || rows.empty()) {
    return;
  }
  SongList songs;
  for (int row : rows) {
    if (row >= 0 && row < source->row_count()) {
      songs.push_back(source->song(row));
    }
  }
  if (songs.empty()) {
    return;
  }
  if (dest_pos < 0) {
    dest->AppendSongs(songs);
  } else {
    dest->InsertSongs(dest_pos, songs);
  }
  source->RemoveRows(rows);
  Persist(dest);
  Persist(source);
}

bool PlaylistManager::UndoCrossMove(int source_id, int dest_id) {
  if (!PlaylistCrossUndoPair::ShouldPairUndo(source_id, dest_id)) {
    return false;
  }
  Playlist *source = FindById(source_id);
  Playlist *dest = FindById(dest_id);
  if (!PlaylistCrossUndoPair::UndoBoth(source, dest)) {
    return false;
  }
  Persist(dest);
  Persist(source);
  return true;
}

void PlaylistManager::InsertUrls(const std::vector<std::string> &urls, int row) {
  Playlist *playlist = Visible();
  if (!playlist) {
    return;
  }
  auto *inserter = new SongLoaderInserter(tagreader_, task_manager_, url_handlers_, collection_backend_);
  inserter->Start(playlist, urls, row, [this, playlist]() { Persist(playlist); });
}

void PlaylistManager::RemoveCurrentSong() {
  Playlist *playlist = Playing();
  if (!playlist || playlist->current_row() < 0) {
    return;
  }
  playlist->RemoveRows({playlist->current_row()});
  Persist(playlist);
}

void PlaylistManager::RefillDynamic() {
  if (Playlist *playlist = Playing()) {
    if (playlist->is_dynamic()) {
      if (playlist->dynamic_generator() && collection_backend_) {
        playlist->dynamic_generator()->set_collection_backend(collection_backend_);
      }
      playlist->RefillDynamic(collection_backend_ ? collection_backend_->Songs() : SongList{});
    }
  }
}

void PlaylistManager::ExpandDynamic() {
  if (Playlist *playlist = Visible()) {
    if (playlist->is_dynamic() && collection_backend_) {
      playlist->ExpandDynamic(collection_backend_->Songs());
      Persist(playlist);
    }
  }
}

void PlaylistManager::RepopulateDynamic() {
  if (Playlist *playlist = Visible()) {
    if (playlist->is_dynamic() && collection_backend_) {
      playlist->RepopulateDynamic(collection_backend_->Songs());
      Persist(playlist);
    }
  }
}

void PlaylistManager::TurnOffDynamic() {
  if (Playlist *playlist = Visible()) {
    playlist->SetDynamic(false);
    Persist(playlist);
  }
}

void PlaylistManager::SaveActive() {
  FlushPendingSaves();
  PersistNow(Playing());
}

void PlaylistManager::SaveCurrent() {
  FlushPendingSaves();
  PersistNow(Visible());
}

void PlaylistManager::ClearCurrent() {
  if (Playlist *playlist = Visible()) {
    playlist->Clear();
    Persist(playlist);
  }
}

void PlaylistManager::ShuffleCurrent() {
  if (Playlist *playlist = Visible()) {
    playlist->Shuffle();
    Persist(playlist);
  }
}

void PlaylistManager::RemoveDuplicatesCurrent() {
  if (Playlist *playlist = Visible()) {
    playlist->RemoveDuplicates();
    Persist(playlist);
  }
}

void PlaylistManager::RemoveUnavailableCurrent() {
  if (Playlist *playlist = Visible()) {
    playlist->RemoveUnavailable();
    Persist(playlist);
  }
}

void PlaylistManager::SongChangeRequestProcessed(const std::string &url, bool valid) {
  for (const auto &playlist : playlists_) {
    if (playlist->ApplyValidityOnCurrentSong(url, valid)) {
      Persist(playlist.get());
      return;
    }
  }
}

void PlaylistManager::RateCurrentSong(float rating) {
  if (Playlist *playlist = Playing()) {
    playlist->RateCurrentSong(rating);
    SchedulePersist(playlist, PlaylistSaveSchedule::Intent::Items);
    const Song song = playlist->current_song();
    if (collection_backend_ && PlaylistRating::ShouldWriteCollectionRating(song)) {
      collection_backend_->SetRating(song.id(), rating);
    }
  }
}

void PlaylistManager::RateCurrentSong2(int rating) {
  const int clamped = std::clamp(rating, 0, 5);
  RateCurrentSong(static_cast<float>(clamped) / 5.0f);
}

void PlaylistManager::PlaySmartPlaylist(const std::string &name, bool as_new, bool clear) {
  SmartPlaylistsModel model;
  model.Reload();
  const SmartPlaylistsItem *item = model.ItemByKey(name);
  if (!item) {
    for (const SmartPlaylistsItem &candidate : model.items()) {
      if (candidate.title == name || candidate.key == name) {
        item = &candidate;
        break;
      }
    }
  }
  SmartPlaylistSearch search = item ? item->search : SmartPlaylistSearch();
  const std::string title = item ? item->title : name;
  auto generator = std::make_shared<PlaylistQueryGenerator>(title, search, true);
  generator->set_collection_backend(collection_backend_);
  Playlist *playlist = nullptr;
  if (as_new || !Visible()) {
    playlist = New(title);
  } else {
    playlist = Visible();
    if (clear) {
      playlist->Clear();
    }
  }
  if (as_new || clear) {
    playlist->SetDynamicGenerator(generator);
  }
  PlaylistGeneratorInserter inserter;
  inserter.Insert(playlist, generator);
  Persist(playlist);
}

void PlaylistManager::SetActivePlaying() {
  if (active_) {
    ActiveChanged.Emit(active_);
  }
}

void PlaylistManager::SetActivePaused() {
  if (active_) {
    ActiveChanged.Emit(active_);
  }
}

void PlaylistManager::SetActiveStopped() {
  if (active_) {
    ActiveChanged.Emit(active_);
  }
}

void PlaylistManager::CycleRepeatMode() {
  Playlist *playlist = Visible();
  if (!playlist) {
    return;
  }
  PlaylistSequence sequence;
  sequence.SetRepeatMode(playlist->repeat_mode());
  sequence.CycleRepeatMode();
  playlist->SetRepeatMode(sequence.repeat_mode());
  SequenceChanged.Emit();
}

void PlaylistManager::CycleShuffleMode() {
  Playlist *playlist = Visible();
  if (!playlist) {
    return;
  }
  PlaylistSequence sequence;
  sequence.SetShuffleMode(playlist->shuffle_mode());
  sequence.CycleShuffleMode();
  playlist->SetShuffleMode(sequence.shuffle_mode());
  if (sequence.shuffle_mode() == PlaylistSequence::ShuffleMode::All) {
    ShuffleCurrent();
  }
  SequenceChanged.Emit();
}

void PlaylistManager::Persist(Playlist *playlist) { SchedulePersist(playlist, PlaylistSaveSchedule::Intent::Full); }

void PlaylistManager::PersistNow(Playlist *playlist) {
  if (playlist && backend_) {
    backend_->SavePlaylist(playlist);
  }
}

void PlaylistManager::PersistLastPlayed(Playlist *playlist) {
  SchedulePersist(playlist, PlaylistSaveSchedule::Intent::LastPlayed);
}

void PlaylistManager::SchedulePersist(Playlist *playlist, PlaylistSaveSchedule::Intent intent) {
  if (!playlist || !backend_) {
    return;
  }
  if (!PlaylistSaveSchedule::ShouldSchedule(playlist->loading(), playlist->id() >= 0)) {
    PersistNow(playlist);
    return;
  }
  pending_intent_ = PlaylistSaveSchedule::Merge(pending_intent_, intent);
  pending_ids_.insert(playlist->id());
  ArmSaveTimer();
}

void PlaylistManager::ArmSaveTimer() {
  if (save_timeout_id_ != 0) {
    return;
  }
  save_timeout_id_ = g_timeout_add_full(
      G_PRIORITY_DEFAULT, PlaylistSaveSchedule::kDelayMs,
      +[](gpointer data) -> gboolean {
        auto *self = static_cast<PlaylistManager *>(data);
        self->save_timeout_id_ = 0;
        self->FlushPendingSaves();
        return G_SOURCE_REMOVE;
      },
      this, nullptr);
}

void PlaylistManager::FlushPendingSaves() {
  if (save_timeout_id_ != 0) {
    g_source_remove(save_timeout_id_);
    save_timeout_id_ = 0;
  }
  const PlaylistSaveSchedule::Intent intent = pending_intent_;
  const std::set<int> ids = pending_ids_;
  pending_intent_ = PlaylistSaveSchedule::Intent::None;
  pending_ids_.clear();
  for (int id : ids) {
    Playlist *playlist = FindById(id);
    if (!playlist) {
      continue;
    }
    if (intent == PlaylistSaveSchedule::Intent::LastPlayed) {
      backend_->SaveLastPlayed(playlist->id(), playlist->current_row());
    } else if (intent == PlaylistSaveSchedule::Intent::Items) {
      backend_->SavePlaylistItems(playlist->id(), playlist->uuids(), playlist->songs());
    } else {
      PersistNow(playlist);
    }
  }
}

Playlist *PlaylistManager::FindByName(const std::string &name) const {
  for (const auto &playlist : playlists_) {
    if (playlist->name() == name) {
      return playlist.get();
    }
  }
  return nullptr;
}

Playlist *PlaylistManager::FindById(int id) const {
  for (const auto &playlist : playlists_) {
    if (playlist->id() == id) {
      return playlist.get();
    }
  }
  return nullptr;
}
