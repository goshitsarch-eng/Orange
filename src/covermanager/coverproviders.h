#ifndef STRAWBERRY_COVERPROVIDERS_H
#define STRAWBERRY_COVERPROVIDERS_H

#include "core/network.h"
#include "core/signal.h"
#include "core/song.h"
#include "covermanager/coverprovider.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CoverProviders {
 public:
  explicit CoverProviders(NetworkAccessManager *network);
  void ReloadSettings();
  void Move(int index, int delta);
  void SaveOrder();
  void Fetch(const Song &song, CoverProvider::Callback callback);
  void FetchAll(const Song &song, const std::function<void(const std::string &provider, const std::string &image_data)> &callback);
  std::vector<CoverProvider *> All() const;
  void FetchFromEmbeddedOrFile(const Song &song, CoverProvider::Callback callback);
  static bool SaveAlbumCover(const Song &song, const std::string &image_data, class TagReader *tagreader = nullptr);

 private:
  void FetchFromIndex(const Song &song, size_t index, CoverProvider::Callback callback);

  NetworkAccessManager *network_;
  std::vector<std::unique_ptr<CoverProvider>> providers_;
};

#endif
