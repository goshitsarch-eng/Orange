#include "core/songloader.h"

#include "collection/collectionbackend.h"
#include "core/urlhandlers.h"
#include "playlistparsers/playlistparser.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

SongLoader::SongLoader(UrlHandlers *url_handlers, CollectionBackend *collection_backend, TagReader *tagreader)
    : url_handlers_(url_handlers), collection_backend_(collection_backend), tagreader_(tagreader) {}

SongLoader::Result SongLoader::Load(const std::string &url) {
  if (url.empty()) {
    errors_.push_back("Empty URL");
    return Result::Error;
  }
  if (url_handlers_) {
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(url)) {
      const UrlHandler::LoadResult loaded = handler->Load(url);
      if (loaded.type == UrlHandler::LoadResult::Type::TrackAvailable) {
        Song song = loaded.song;
        if (!song.is_valid()) {
          song.set_url(loaded.media_url.empty() ? url : loaded.media_url);
          song.set_stream_url(loaded.stream_url);
          song.set_valid(true);
        }
        songs_.push_back(song);
        return Result::Success;
      }
      if (loaded.type == UrlHandler::LoadResult::Type::Error) {
        errors_.push_back(loaded.error.empty() ? url : loaded.error);
        return Result::Error;
      }
    }
  }
  const std::string path = FileUtils::PathFromUri(url);
  if (FileUtils::Exists(path)) {
    return LoadLocal(path);
  }
  AddRawStream(url);
  return Result::Success;
}

SongLoader::Result SongLoader::LoadMany(const std::vector<std::string> &urls) {
  bool blocking = false;
  bool error = false;
  for (const std::string &url : urls) {
    const Result result = Load(url);
    if (result == Result::BlockingLoadRequired) {
      blocking = true;
    }
    if (result == Result::Error) {
      error = true;
    }
  }
  if (blocking || !pending_paths_.empty()) {
    return Result::BlockingLoadRequired;
  }
  if (songs_.empty()) {
    return error ? Result::Error : Result::Error;
  }
  return Result::Success;
}

SongLoader::Result SongLoader::LoadFilenamesBlocking() {
  const std::vector<std::string> queued = pending_paths_;
  pending_paths_.clear();
  for (const std::string &path : queued) {
    if (FileUtils::IsDirectory(path)) {
      LoadLocalDirectory(path);
    } else {
      LoadLocal(path);
    }
  }
  return songs_.empty() ? Result::Error : Result::Success;
}

void SongLoader::LoadMetadataBlocking() {
  for (Song &song : songs_) {
    EffectiveSongLoad(&song);
  }
}

SongLoader::Result SongLoader::LoadAudioCD() {
  errors_.push_back("Audio CD loading is handled by the device manager");
  return Result::Error;
}

SongLoader::Result SongLoader::LoadLocal(const std::string &path) {
  if (FileUtils::IsDirectory(path)) {
    pending_paths_.push_back(path);
    return Result::BlockingLoadRequired;
  }
  if (PlaylistParser::IsPlaylist(path)) {
    LoadPlaylistFile(path);
    return songs_.empty() ? Result::Error : Result::Success;
  }
  if (Song::IsAudioFile(path)) {
    LoadAudioFile(path);
    return Result::Success;
  }
  errors_.push_back("Unsupported file: " + path);
  return Result::Error;
}

void SongLoader::LoadLocalDirectory(const std::string &path) {
  for (const std::string &entry : FileUtils::ListDirectoryRecursive(path)) {
    if (PlaylistParser::IsPlaylist(entry) || Song::IsAudioFile(entry)) {
      LoadLocal(entry);
    }
  }
}

void SongLoader::LoadPlaylistFile(const std::string &path) {
  playlist_name_ = FileUtils::BaseName(path);
  PlaylistParser parser;
  SongList loaded = parser.Load(path);
  if (StrUtils::ToLower(FileUtils::Extension(path)) == "cue" && !loaded.empty() && tagreader_) {
    const std::string audio = FileUtils::PathFromUri(loaded.front().url());
    if (FileUtils::IsFile(audio)) {
      PlaylistParser::EnrichFromAudioFile(&loaded, tagreader_->ReadFile(audio));
    }
  }
  songs_.insert(songs_.end(), loaded.begin(), loaded.end());
}

void SongLoader::LoadAudioFile(const std::string &path) {
  const std::string cue = PlaylistParser::FindCueForAudio(path);
  if (!cue.empty()) {
    LoadPlaylistFile(cue);
    return;
  }
  if (collection_backend_) {
    Song collection = collection_backend_->SongByUrl(FileUtils::UriFromPath(path));
    if (collection.is_valid()) {
      songs_.push_back(collection);
      return;
    }
  }
  Song song;
  if (tagreader_) {
    song = tagreader_->ReadFile(path);
  }
  if (!song.is_valid()) {
    song.set_url(FileUtils::UriFromPath(path));
    song.set_title(FileUtils::BaseName(path));
    song.set_valid(true);
  }
  songs_.push_back(song);
}

void SongLoader::AddRawStream(const std::string &url) {
  Song song(Song::Source::Stream);
  song.set_url(url);
  song.set_title(url);
  song.set_valid(true);
  songs_.push_back(song);
}

void SongLoader::EffectiveSongLoad(Song *song) {
  if (!song || !tagreader_) {
    return;
  }
  const std::string path = FileUtils::PathFromUri(song->url());
  if (!FileUtils::IsFile(path)) {
    return;
  }
  Song loaded = tagreader_->ReadFile(path);
  if (loaded.is_valid()) {
    *song = loaded;
  }
}
