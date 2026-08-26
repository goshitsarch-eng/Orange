#ifndef STRAWBERRY_DEEZERCOVERPROVIDER_H
#define STRAWBERRY_DEEZERCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class DeezerCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
  };

  static const char *kApiUrl;
  static const int kLimit;

  std::string name() const override { return "Deezer"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;

  static std::string SearchUrl(const std::string &artist, const std::string &album, const std::string &title);
  static std::vector<SearchResult> ParseResults(const std::string &json);
};

#endif
