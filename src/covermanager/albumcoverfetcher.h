#ifndef STRAWBERRY_ALBUMCOVERFETCHER_H
#define STRAWBERRY_ALBUMCOVERFETCHER_H

#include "core/signal.h"
#include "covermanager/albumcoverimageresult.h"
#include "covermanager/coversearchstatistics.h"

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
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
  std::string image_data;
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
  ~AlbumCoverFetcher();

  uint64_t SearchForCovers(const std::string &artist, const std::string &album, const std::string &title = {});
  uint64_t FetchAlbumCover(const std::string &artist, const std::string &album, const std::string &title, bool batch);
  void Clear();
  uint64_t next_id() const { return next_id_; }
  size_t queued() const { return queued_.size(); }
  size_t active() const { return active_.size(); }

  Signal<uint64_t, CoverProviderSearchResults, CoverSearchStatistics> SearchFinished;
  Signal<uint64_t, AlbumCoverImageResult, CoverSearchStatistics> AlbumCoverFetched;

 private:
  struct Job {
    CoverSearchRequest request;
    CoverProviderSearchResults results;
    CoverSearchStatistics statistics;
    int pending = 0;
    bool cancelled = false;
    bool finished = false;
  };

  void AddRequest(const CoverSearchRequest &request);
  void StartRequests();
  void StartJob(const CoverSearchRequest &request);
  void FinishJob(const std::shared_ptr<Job> &job);
  void FetchBestCover(const std::shared_ptr<Job> &job);

  CoverProviders *cover_providers_;
  NetworkAccessManager *network_;
  uint64_t next_id_ = 1;
  std::queue<CoverSearchRequest> queued_;
  std::map<uint64_t, std::shared_ptr<Job>> active_;
};

#endif
