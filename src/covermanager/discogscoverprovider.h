#ifndef STRAWBERRY_DISCOGSCOVERPROVIDER_H
#define STRAWBERRY_DISCOGSCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class DiscogsCoverProvider : public CoverProvider {
 public:
  struct SearchHit {
    std::string title;
    std::string resource_url;
    std::string id;
  };

  struct ImageResult {
    std::string artist;
    std::string album;
    std::string image_url;
  };

  static const char *kUrlSearch;
  static const char *kAccessKeyB64;
  static const char *kSecretKeyB64;

  std::string name() const override { return "Discogs"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;

  static std::string AccessKey();
  static std::string SecretKey();
  static std::string SearchUrl(const std::string &artist, const std::string &album, const std::string &type = "master");
  static std::vector<SearchHit> ParseSearchResults(const std::string &json, const std::string &artist, const std::string &album);
  static std::vector<ImageResult> ParseReleaseImages(const std::string &json, const std::string &search_artist, const std::string &search_album);
  static bool AcceptImage(int width, int height);
};

#endif
