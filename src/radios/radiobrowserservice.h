#ifndef STRAWBERRY_RADIOBROWSERSERVICE_H
#define STRAWBERRY_RADIOBROWSERSERVICE_H

#include "radios/radiochannel.h"

#include <string>
#include <vector>

class RadioBrowserService {
 public:
  struct Country {
    std::string name;
    std::string code;
    int stationcount = 0;
  };

  struct StationPage {
    std::vector<RadioChannel> channels;
    int raw_count = 0;
  };

  static const std::vector<std::string> kServers;

  static std::string Homepage();
  static std::string Donate();
  static std::string DefaultServer();
  static std::string SearchUrl(const std::string &server, const std::string &query, const std::string &country = {},
                               bool hide_broken = true, int limit = 50, int offset = 0, const std::string &order = "votes");
  static std::string CountriesUrl(const std::string &server);
  static std::vector<RadioChannel> ParseStations(const std::string &json);
  static StationPage ParseStationPage(const std::string &json);
  static std::vector<Country> ParseCountries(const std::string &json);
};

#endif
