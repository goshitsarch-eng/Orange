#ifndef STRAWBERRY_TIDALCOVERPROVIDER_H
#define STRAWBERRY_TIDALCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class TidalCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
  };

  static const int kLimit;

  std::string name() const override { return "Tidal"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string ImageUrl(const std::string &cover, const std::string &size = "1280x1280");
  static std::string SearchUrl(const std::string &artist, const std::string &album, const std::string &title, const std::string &country = "US");
  static std::vector<SearchResult> ParseItems(const std::string &json);
};

#endif
