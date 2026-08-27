#include "covermanager/albumcoverfetcher.h"

#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/coverfetchpolicy.h"
#include "covermanager/coverproviders.h"
#include "utilities/jsonutils.h"

#include <algorithm>

AlbumCoverFetcher::AlbumCoverFetcher(CoverProviders *cover_providers, NetworkAccessManager *network)
    : cover_providers_(cover_providers), network_(network) {}

AlbumCoverFetcher::~AlbumCoverFetcher() { Clear(); }

uint64_t AlbumCoverFetcher::SearchForCovers(const std::string &artist, const std::string &album, const std::string &title) {
  const CoverSearchRequest request = AlbumCoverFetcherSearch::MakeRequest(next_id_++, artist, album, title, true, false);
  AddRequest(request);
  return request.id;
}

uint64_t AlbumCoverFetcher::FetchAlbumCover(const std::string &artist, const std::string &album, const std::string &title, bool batch) {
  const CoverSearchRequest request = AlbumCoverFetcherSearch::MakeRequest(next_id_++, artist, album, title, false, batch);
  AddRequest(request);
  return request.id;
}

void AlbumCoverFetcher::AddRequest(const CoverSearchRequest &request) {
  if (!cover_providers_ || !network_) {
    return;
  }
  queued_.push(request);
  StartRequests();
}

void AlbumCoverFetcher::Clear() {
  while (!queued_.empty()) {
    queued_.pop();
  }
  for (auto &entry : active_) {
    if (entry.second) {
      entry.second->cancelled = true;
    }
  }
  active_.clear();
}

void AlbumCoverFetcher::StartRequests() {
  while (!queued_.empty() && static_cast<int>(active_.size()) < AlbumCoverFetcherSearch::kMaxConcurrentRequests) {
    const CoverSearchRequest request = queued_.front();
    queued_.pop();
    StartJob(request);
  }
}

void AlbumCoverFetcher::StartJob(const CoverSearchRequest &request) {
  auto job = std::make_shared<Job>();
  job->request = request;
  if (AlbumCoverFetcherSearch::ShouldTerminateSearch(request)) {
    FinishJob(job);
    return;
  }
  const std::vector<CoverProvider *> providers = cover_providers_->All();
  for (CoverProvider *provider : providers) {
    if (AlbumCoverFetcherSearch::ShouldUseProvider(provider, request)) {
      ++job->pending;
    }
  }
  if (job->pending == 0) {
    FinishJob(job);
    return;
  }
  active_[request.id] = job;
  const Song song = AlbumCoverFetcherSearch::SongFromRequest(request);
  for (CoverProvider *provider : providers) {
    if (!AlbumCoverFetcherSearch::ShouldUseProvider(provider, request)) {
      continue;
    }
    ++job->statistics.network_requests_made;
    provider->Search(song, network_, [this, job, provider](const CoverProviderSearchResults &found) {
      if (job->cancelled || job->finished) {
        return;
      }
      CoverProviderSearchResults scored = found;
      AlbumCoverFetcherSearch::ScoreResults(job->request, provider->quality(), provider->name(), &scored);
      job->results.insert(job->results.end(), scored.begin(), scored.end());
      job->statistics.total_images_by_provider[provider->name()] += found.size();
      job->statistics.bytes_transferred += found.size();
      for (const CoverProviderSearchResult &result : scored) {
        job->best_score = std::max(job->best_score, result.score());
      }
      if (--job->pending <= 0 || CoverFetchPolicy::ShouldStop(job->best_score, job->request.search)) {
        FinishJob(job);
      }
    });
  }
}

void AlbumCoverFetcher::FinishJob(const std::shared_ptr<Job> &job) {
  if (!job || job->finished || job->cancelled) {
    return;
  }
  job->finished = true;
  AlbumCoverFetcherSearch::SortByScore(&job->results);
  AlbumCoverFetcherSearch::AssignNumbers(&job->results);
  active_.erase(job->request.id);
  if (job->request.search) {
    if (job->results.empty()) {
      ++job->statistics.missing_images;
    }
    SearchFinished.Emit(job->request.id, job->results, job->statistics);
    StartRequests();
    return;
  }
  FetchBestCover(job);
}

void AlbumCoverFetcher::FetchBestCover(const std::shared_ptr<Job> &job) {
  const CoverProviderSearchResult *best = AlbumCoverFetcherSearch::Best(job->results);
  if (!best) {
    ++job->statistics.missing_images;
    AlbumCoverFetched.Emit(job->request.id, {}, job->statistics);
    StartRequests();
    return;
  }
  if (!best->image_data.empty()) {
    AlbumCoverImageResult image;
    image.image_data.assign(best->image_data.begin(), best->image_data.end());
    image.image_url = best->image_url;
    image.width = best->image_width;
    image.height = best->image_height;
    ++job->statistics.chosen_images;
    job->statistics.chosen_images_by_provider[best->provider] += 1;
    AlbumCoverFetched.Emit(job->request.id, image, job->statistics);
    StartRequests();
    return;
  }
  if (!network_ || !AlbumCoverFetcherSearch::IsHttpUrl(best->image_url)) {
    ++job->statistics.missing_images;
    AlbumCoverFetched.Emit(job->request.id, {}, job->statistics);
    StartRequests();
    return;
  }
  ++job->statistics.network_requests_made;
  const std::string provider = best->provider;
  const std::string image_url = best->image_url;
  network_->Get(image_url, [this, job, provider, image_url](const NetworkAccessManager::Response &response) {
    if (job->cancelled) {
      return;
    }
    AlbumCoverImageResult image;
    if (response.ok() && JsonUtils::LooksLikeImage(response.body)) {
      image.image_data.assign(response.body.begin(), response.body.end());
      image.image_url = image_url;
      job->statistics.bytes_transferred += response.body.size();
      ++job->statistics.chosen_images;
      job->statistics.chosen_images_by_provider[provider] += 1;
    } else {
      ++job->statistics.missing_images;
    }
    AlbumCoverFetched.Emit(job->request.id, image, job->statistics);
    StartRequests();
  });
}
