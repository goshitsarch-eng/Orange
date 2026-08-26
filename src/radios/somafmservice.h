#ifndef STRAWBERRY_SOMAFMSERVICE_H
#define STRAWBERRY_SOMAFMSERVICE_H

#include "radios/radioservices.h"

#include <string>
#include <vector>

class SomaFMService {
 public:
  static const char *kApiChannelsUrl;
  static const char *kQualityDefault;

  static std::string Homepage();
  static std::string Donate();
  static std::string NormalizeQuality(const std::string &quality);
  static std::vector<RadioChannel> ParseChannels(const std::string &json, const std::string &quality);
};

#endif
