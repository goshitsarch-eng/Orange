#ifndef STRAWBERRY_RADIOMODEL_H
#define STRAWBERRY_RADIOMODEL_H

#include "radios/radiochannel.h"

#include <string>
#include <vector>

class RadioModel {
 public:
  void SetChannels(const std::vector<RadioChannel> &channels);
  void SetSearchResults(const std::vector<RadioChannel> &channels);
  const std::vector<RadioChannel> &visible() const { return visible_; }
  std::string Label(const RadioChannel &channel) const;
  int row_count() const { return static_cast<int>(visible_.size()); }

 private:
  std::vector<RadioChannel> visible_;
};

#endif
