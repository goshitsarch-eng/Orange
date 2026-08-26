#ifndef STRAWBERRY_ALBUMCOVERFETCHER_H
#define STRAWBERRY_ALBUMCOVERFETCHER_H

#include "covermanager/albumcoverimageresult.h"
#include "covermanager/coversearchstatistics.h"

#include <cstdint>
#include <string>
#include <vector>

struct CoverSearchRequest {
  uint64_t id = 0;
  std::string artist;
  std::string album;
  std::string title;
  bool search = false;
  bool batch = false;
};

struct CoverProviderSearchResult {
  std::string provider;
  std::string artist;
  std::string album;
  std::string image_url;
  int image_width = 0;
  int image_height = 0;
  float score_provider = 0.0f;
  float score_match = 0.0f;
  float score_quality = 0.0f;
  int number = 0;

  float score() const { return score_provider + score_match + score_quality; }
};

using CoverProviderSearchResults = std::vector<CoverProviderSearchResult>;

class CoverProviders;
class NetworkAccessManager;

class AlbumCoverFetcher {
 public:
  AlbumCoverFetcher(CoverProviders *cover_providers, NetworkAccessManager *network);

  uint64_t SearchForCovers(const std::string &artist, const std::string &album, const std::string &title = {});
  uint64_t FetchAlbumCover(const std::string &artist, const std::string &album, const std::string &title, bool batch);
  void Clear();
  uint64_t next_id() const { return next_id_; }

 private:
  CoverProviders *cover_providers_;
  NetworkAccessManager *network_;
  uint64_t next_id_ = 1;
};

#endif
