#ifndef STRAWBERRY_COVERPROVIDERS_H
#define STRAWBERRY_COVERPROVIDERS_H

#include "core/network.h"
#include "core/signal.h"
#include "core/song.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CoverProvider {
 public:
  using Callback = std::function<void(const std::string &image_data, const std::string &error)>;
  virtual ~CoverProvider() = default;
  virtual std::string name() const = 0;
  virtual bool enabled() const { return enabled_; }
  virtual void set_enabled(bool enabled) { enabled_ = enabled; }
  virtual void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) = 0;
 protected:
  bool enabled_ = true;
};

class CoverProviders {
 public:
  explicit CoverProviders(NetworkAccessManager *network);
  void ReloadSettings();
  void Fetch(const Song &song, CoverProvider::Callback callback);
  std::vector<CoverProvider *> All() const;
  void FetchFromEmbeddedOrFile(const Song &song, CoverProvider::Callback callback);

 private:
  void FetchFromIndex(const Song &song, size_t index, CoverProvider::Callback callback);

  NetworkAccessManager *network_;
  std::vector<std::unique_ptr<CoverProvider>> providers_;
};

class AlbumCoverLoader {
 public:
  explicit AlbumCoverLoader(class TagReader *tagreader);
  std::string LoadPath(const Song &song) const;
  std::vector<unsigned char> LoadData(const Song &song) const;
 private:
  class TagReader *tagreader_;
};

class CurrentAlbumCoverLoader {
 public:
  explicit CurrentAlbumCoverLoader(AlbumCoverLoader *loader);
  void Load(const Song &song);
  const std::vector<unsigned char> &current() const { return current_; }
  Signal<Song, std::vector<unsigned char>> AlbumCoverReady;
 private:
  AlbumCoverLoader *loader_;
  std::vector<unsigned char> current_;
};

#endif
