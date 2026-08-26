#include "playlist/playlistmanager.h"

#include "collection/collectionbackend.h"
#include "playlistparsers/playlistparser.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

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
  active_ = nullptr;
  if (backend_) {
    for (const PlaylistMetadata &metadata : backend_->GetAllPlaylists()) {
      playlists_.push_back(backend_->LoadPlaylist(metadata.id));
    }
  }
  if (playlists_.empty()) {
    New("Playlist");
  } else {
    active_ = playlists_.front().get();
  }
  PlaylistsLoaded.Emit();
}

Playlist *PlaylistManager::New(const std::string &name) {
  auto playlist = std::make_unique<Playlist>();
  playlist->set_name(name);
  if (backend_) {
    backend_->SavePlaylist(playlist.get());
  }
  active_ = playlist.get();
  playlists_.push_back(std::move(playlist));
  PlaylistAdded.Emit(active_);
  CurrentChanged.Emit(active_);
  return active_;
}

void PlaylistManager::Close(int id) {
  playlists_.erase(std::remove_if(playlists_.begin(), playlists_.end(),
                                  [id](const std::unique_ptr<Playlist> &playlist) { return playlist->id() == id; }),
                   playlists_.end());
  if (backend_) {
    backend_->DeletePlaylist(id);
  }
  if (active_ && active_->id() == id) {
    active_ = playlists_.empty() ? nullptr : playlists_.front().get();
  }
  PlaylistClosed.Emit(id);
}

void PlaylistManager::SetCurrentPlaylist(const std::string &name) {
  if (Playlist *playlist = FindByName(name)) {
    active_ = playlist;
    CurrentChanged.Emit(active_);
  }
}

void PlaylistManager::SetCurrentRow(int row) {
  if (active_) {
    active_->set_current_row(row);
  }
}

int PlaylistManager::current_row() const { return active_ ? active_->current_row() : -1; }

Song PlaylistManager::current_song() const { return active_ ? active_->current_song() : Song(); }

void PlaylistManager::Next() {
  if (active_) {
    active_->Next();
  }
}

void PlaylistManager::Previous() {
  if (active_) {
    active_->Previous();
  }
}

void PlaylistManager::AppendSongs(const SongList &songs) {
  if (active_) {
    active_->AppendSongs(songs);
    SaveActive();
  }
}

void PlaylistManager::InsertUrls(const std::vector<std::string> &urls, int row) {
  if (!active_ || !tagreader_) {
    return;
  }
  SongList songs;
  PlaylistParser parser;
  auto enrich_cue = [&](SongList loaded) {
    if (!loaded.empty()) {
      const std::string audio = FileUtils::PathFromUri(loaded.front().url());
      if (FileUtils::IsFile(audio)) {
        PlaylistParser::EnrichFromAudioFile(&loaded, tagreader_->ReadFile(audio));
      }
    }
    return loaded;
  };
  for (const std::string &url : urls) {
    const std::string path = FileUtils::PathFromUri(url);
    if (FileUtils::IsFile(path) && PlaylistParser::IsPlaylist(path)) {
      SongList loaded = parser.Load(path);
      if (StrUtils::ToLower(FileUtils::Extension(path)) == "cue") {
        loaded = enrich_cue(loaded);
      }
      songs.insert(songs.end(), loaded.begin(), loaded.end());
      continue;
    }
    if (FileUtils::IsFile(path) && Song::IsAudioFile(path)) {
      const std::string cue = PlaylistParser::FindCueForAudio(path);
      if (!cue.empty()) {
        SongList loaded = enrich_cue(parser.Load(cue));
        songs.insert(songs.end(), loaded.begin(), loaded.end());
        continue;
      }
      songs.push_back(tagreader_->ReadFile(path));
      continue;
    }
    Song song(Song::Source::Stream);
    song.set_url(url);
    song.set_title(url);
    song.set_valid(true);
    songs.push_back(song);
  }
  if (row < 0) {
    active_->AppendSongs(songs);
  } else {
    active_->InsertSongs(row, songs);
  }
  SaveActive();
}

void PlaylistManager::RefillDynamic() {
  if (active_ && active_->is_dynamic() && collection_backend_) {
    active_->RefillDynamic(collection_backend_->Songs());
  }
}

void PlaylistManager::SaveActive() {
  if (active_ && backend_) {
    backend_->SavePlaylist(active_);
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
