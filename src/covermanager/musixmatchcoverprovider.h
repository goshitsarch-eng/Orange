#ifndef STRAWBERRY_MUSIXMATCHCOVERPROVIDER_H
#define STRAWBERRY_MUSIXMATCHCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class MusixmatchCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
  };

  std::string name() const override { return "Musixmatch"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string AlbumUrl(const std::string &artist, const std::string &album);
  static std::string ExtractNextDataJson(const std::string &html);
  static std::vector<SearchResult> ParseAlbumPage(const std::string &html, const std::string &artist, const std::string &album);
};

#endif
