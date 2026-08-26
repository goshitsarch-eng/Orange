#include "playlist/playlistmanager.h"

#include "collection/collectionbackend.h"
#include "core/songloader.h"
#include "playlistparsers/playlistparser.h"
#include "smartplaylists/playlistgeneratorinserter.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "smartplaylists/smartplaylistsmodel.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <algorithm>

PlaylistManager::PlaylistManager(TaskManager *task_manager, TagReader *tagreader, UrlHandlers *url_handlers, PlaylistBackend *backend,
                                 CollectionBackend *collection_backend)
    : task_manager_(task_manager),
      tagreader_(tagreader),
      url_handlers_(url_handlers),
      backend_(backend),
      collection_backend_(collection_backend) {}

void PlaylistManager::Init() { LoadAll(); }

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
      if (playlist->id() >= next_id_) {
        next_id_ = playlist->id() + 1;
      }
    }
  }
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
  Persist(playlist.get());
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
  PlaylistAdded.Emit(current_);
  CurrentChanged.Emit(current_);
  return current_;
}

void PlaylistManager::Load(const std::string &filename) {
  const SongList songs = PlaylistParser().Load(filename);
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

void PlaylistManager::Delete(int id) {
  Close(id);
  if (backend_) {
    backend_->DeletePlaylist(id);
  }
  PlaylistDeleted.Emit(id);
}

bool PlaylistManager::Close(int id) {
  if (!FindById(id)) {
    return false;
  }
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
    playlist->set_current_row(row);
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
  }
}

void PlaylistManager::Previous() {
  if (Playlist *playlist = Playing()) {
    playlist->Previous();
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

void PlaylistManager::InsertUrls(const std::vector<std::string> &urls, int row) {
  Playlist *playlist = Visible();
  if (!playlist) {
    return;
  }
  SongLoader loader(url_handlers_, collection_backend_, tagreader_);
  const SongLoader::Result result = loader.LoadMany(urls);
  if (result == SongLoader::Result::BlockingLoadRequired) {
    loader.LoadFilenamesBlocking();
  }
  loader.LoadMetadataBlocking();
  const SongList &songs = loader.songs();
  if (songs.empty()) {
    return;
  }
  if (row < 0) {
    playlist->AppendSongs(songs);
  } else {
    playlist->InsertSongs(row, songs);
  }
  Persist(playlist);
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
    if (playlist->is_dynamic() && collection_backend_) {
      playlist->RefillDynamic(collection_backend_->Songs());
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

void PlaylistManager::SaveActive() { Persist(Playing()); }

void PlaylistManager::SaveCurrent() { Persist(Visible()); }

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

void PlaylistManager::RateCurrentSong(float rating) {
  if (Playlist *playlist = Playing()) {
    playlist->RateCurrentSong(rating);
    Persist(playlist);
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
    playlist->SetDynamic(true, search);
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

void PlaylistManager::Persist(Playlist *playlist) {
  if (playlist && backend_) {
    backend_->SavePlaylist(playlist);
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
