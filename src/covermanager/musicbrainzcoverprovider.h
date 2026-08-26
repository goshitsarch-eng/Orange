#ifndef STRAWBERRY_MUSICBRAINZCOVERPROVIDER_H
#define STRAWBERRY_MUSICBRAINZCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class MusicbrainzCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
    std::string release_id;
  };

  static const char *kReleaseSearchUrl;
  static const char *kAlbumCoverUrl;
  static const int kLimit;

  std::string name() const override { return "MusicBrainz"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string EscapeQuery(const std::string &value);
  static std::string SearchUrl(const std::string &artist, const std::string &album);
  static std::string CoverArtUrl(const std::string &release_id);
  static std::vector<SearchResult> ParseReleases(const std::string &json, const std::string &search_artist);
};

#endif
