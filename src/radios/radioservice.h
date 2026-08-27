#ifndef STRAWBERRY_RADIOSERVICE_H
#define STRAWBERRY_RADIOSERVICE_H

#include "core/song.h"
#include "radios/radiochannel.h"

#include <functional>
#include <string>
#include <vector>

class RadioService {
 public:
  using Callback = std::function<void(const std::vector<RadioChannel> &)>;
  virtual ~RadioService() = default;
  virtual std::string name() const = 0;
  virtual Song::Source source() const = 0;
  virtual void Fetch(Callback callback) = 0;
};

#endif
