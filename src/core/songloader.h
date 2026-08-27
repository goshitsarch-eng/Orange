#ifndef STRAWBERRY_SONGLOADER_H
#define STRAWBERRY_SONGLOADER_H

#include "core/song.h"

#include <string>
#include <vector>

class CollectionBackend;
class TagReader;
class UrlHandlers;

class SongLoader {
 public:
  enum class Result {
    Success = 0,
    Error = 1,
    BlockingLoadRequired = 2
  };

  SongLoader(UrlHandlers *url_handlers, CollectionBackend *collection_backend, TagReader *tagreader);

  Result Load(const std::string &url);
  Result LoadMany(const std::vector<std::string> &urls);
  Result LoadFilenamesBlocking();
  void LoadMetadataBlocking();
  Result LoadAudioCD();
  Result LoadRemoteFromData(const std::string &url, const std::string &data);

  const SongList &songs() const { return songs_; }
  const std::string &playlist_name() const { return playlist_name_; }
  const std::vector<std::string> &errors() const { return errors_; }

 private:
  Result LoadLocal(const std::string &path);
  Result LoadRemote(const std::string &url);
  void LoadLocalDirectory(const std::string &path);
  void LoadPlaylistFile(const std::string &path);
  void LoadAudioFile(const std::string &path);
  void AddRawStream(const std::string &url);
  void EffectiveSongLoad(Song *song);

  UrlHandlers *url_handlers_ = nullptr;
  CollectionBackend *collection_backend_ = nullptr;
  TagReader *tagreader_ = nullptr;
  SongList songs_;
  std::string playlist_name_;
  std::vector<std::string> errors_;
  std::vector<std::string> pending_paths_;
  std::vector<std::string> pending_remote_urls_;
};

#endif
