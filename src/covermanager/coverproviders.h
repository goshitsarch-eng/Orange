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
  virtual float quality() const { return quality_; }
  virtual void set_quality(float quality) { quality_ = quality; }
  virtual int order() const { return order_; }
  virtual void set_order(int order) { order_ = order; }
  virtual void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) = 0;
 protected:
  bool enabled_ = true;
  float quality_ = 1.0f;
  int order_ = 0;
};

class CoverProviders {
 public:
  explicit CoverProviders(NetworkAccessManager *network);
  void ReloadSettings();
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
