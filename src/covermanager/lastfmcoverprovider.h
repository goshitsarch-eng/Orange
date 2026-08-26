#ifndef STRAWBERRY_LASTFMCOVERPROVIDER_H
#define STRAWBERRY_LASTFMCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <map>
#include <string>
#include <vector>

class LastFmCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
  };

  enum class ImageSize {
    Unknown = 0,
    Small = 1,
    Medium = 2,
    Large = 3,
    ExtraLarge = 4
  };

  static const char *kUrl;
  static const char *kApiKey;
  static const char *kSecret;

  std::string name() const override { return "Last.fm"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string Sign(const std::map<std::string, std::string> &params);
  static std::string FormBody(const std::map<std::string, std::string> &params);
  static ImageSize ImageSizeFromString(const std::string &size);
  static std::string UpgradeImageUrl(const std::string &url);
  static std::vector<SearchResult> ParseResults(const std::string &json, const std::string &type);
};

#endif
