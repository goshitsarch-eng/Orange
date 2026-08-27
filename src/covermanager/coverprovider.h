#ifndef STRAWBERRY_COVERPROVIDER_H
#define STRAWBERRY_COVERPROVIDER_H

#include "core/network.h"
#include "core/song.h"
#include "covermanager/albumcoverfetcher.h"

#include <functional>
#include <string>

class CoverProvider {
 public:
  using Callback = std::function<void(const std::string &image_data, const std::string &error)>;
  using SearchCallback = std::function<void(const CoverProviderSearchResults &results)>;
  virtual ~CoverProvider() = default;
  virtual std::string name() const = 0;
  virtual bool enabled() const { return enabled_; }
  virtual void set_enabled(bool enabled) { enabled_ = enabled; }
  virtual float quality() const { return quality_; }
  virtual void set_quality(float quality) { quality_ = quality; }
  virtual int order() const { return order_; }
  virtual void set_order(int order) { order_ = order; }
  virtual bool authentication_required() const { return authentication_required_; }
  virtual bool authenticated() const { return !authentication_required(); }
  virtual bool batch() const { return batch_; }
  virtual bool allow_missing_album() const { return allow_missing_album_; }
  virtual void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) = 0;
  virtual void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback);

 protected:
  bool enabled_ = true;
  float quality_ = 1.0f;
  int order_ = 0;
  bool authentication_required_ = false;
  bool batch_ = true;
  bool allow_missing_album_ = true;
};

#endif
