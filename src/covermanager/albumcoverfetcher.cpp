#include "covermanager/albumcoverfetcher.h"

#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/coverproviders.h"

AlbumCoverFetcher::AlbumCoverFetcher(CoverProviders *cover_providers, NetworkAccessManager *network)
    : cover_providers_(cover_providers), network_(network) {}

uint64_t AlbumCoverFetcher::SearchForCovers(const std::string &artist, const std::string &album, const std::string &title) {
  (void)artist;
  (void)album;
  (void)title;
  (void)cover_providers_;
  (void)network_;
  return next_id_++;
}

uint64_t AlbumCoverFetcher::FetchAlbumCover(const std::string &artist, const std::string &album, const std::string &title, bool batch) {
  (void)artist;
  (void)album;
  (void)title;
  (void)batch;
  return next_id_++;
}

void AlbumCoverFetcher::Clear() {}
