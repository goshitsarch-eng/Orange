#ifndef STRAWBERRY_SPOTIFYCOVERPROVIDER_H
#define STRAWBERRY_SPOTIFYCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class SpotifyCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
    int width = 0;
    int height = 0;
  };

  static const char *kApiUrl;
  static const int kLimit;

  std::string name() const override { return "Spotify"; }
  bool authentication_required() const override { return true; }
  bool authenticated() const override;
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string SearchUrl(const std::string &artist, const std::string &album, const std::string &title);
  static std::vector<SearchResult> ParseResults(const std::string &json, const std::string &extract);
};

#endif
