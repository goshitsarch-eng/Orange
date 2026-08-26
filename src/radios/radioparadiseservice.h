#ifndef STRAWBERRY_RADIOPARADISESERVICE_H
#define STRAWBERRY_RADIOPARADISESERVICE_H

#include "radios/radioservices.h"

#include <string>
#include <vector>

class RadioParadiseService {
 public:
  static const char *kApiChannelsUrl;

  static std::string Homepage();
  static std::string Donate();
  static std::string EnsureAbsoluteUrl(const std::string &url);
  static std::vector<RadioChannel> ParseChannels(const std::string &json);
};

#endif
