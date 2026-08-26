#ifndef STRAWBERRY_COVERPROVIDER_H
#define STRAWBERRY_COVERPROVIDER_H

#include "core/network.h"
#include "core/song.h"

#include <functional>
#include <string>

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

#endif
