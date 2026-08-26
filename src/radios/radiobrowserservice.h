#ifndef STRAWBERRY_RADIOBROWSERSERVICE_H
#define STRAWBERRY_RADIOBROWSERSERVICE_H

#include "radios/radioservices.h"

#include <string>
#include <vector>

class RadioBrowserService {
 public:
  static const std::vector<std::string> kServers;

  static std::string Homepage();
  static std::string Donate();
  static std::string DefaultServer();
  static std::string SearchUrl(const std::string &server, const std::string &query, const std::string &country = {},
                               bool hide_broken = true, int limit = 50, int offset = 0);
  static std::vector<RadioChannel> ParseStations(const std::string &json);
};

#endif
