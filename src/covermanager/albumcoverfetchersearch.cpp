#include "covermanager/albumcoverfetchersearch.h"

#include "utilities/strutils.h"

#include <algorithm>
#include <cmath>

namespace {

const char *kCompilationKeywords[] = {
    "hits",
    "greatest",
    "best",
    "collection",
    "classics",
    "singles",
    "bootleg",
    "live",
    "concert",
    "essential",
    "ultimate",
    "karaoke",
    "mixtape",
    "country rock",
    "indie folk",
    "soft rock",
    "folk music",
    "60's rock",
    "60's romance",
    "60s music",
    "late 60s",
    "the 60s",
    "folk and blues",
    "60 from the 60's",
    "classic psychedelic",
    "playlist: acoustic",
    "90's rnb playlist",
    "rock 80s",
    "classic 80s",
    "rock anthems",
    "rock songs",
    "rock 2019",
    "guitar anthems",
    "driving anthems",
    "traffic jam jams",
    "perfect background music",
    "70's gold",
    "rockfluence",
    "acoustic dinner accompaniment",
    "complete studio albums",
    "mellow rock",
};

}  // namespace

float AlbumCoverFetcherSearch::ScoreImage(int width, int height) {
  if (width == 0 || height == 0) {
    return 0.0f;
  }
  const float size_score = std::sqrt(static_cast<float>(width * height)) / kTargetSize;
  const float aspect_score = 1.0f - static_cast<float>(std::max(width, height) - std::min(width, height)) / static_cast<float>(std::max(height, width));
  return size_score + aspect_score;
}

bool AlbumCoverFetcherSearch::IsCompilationOrLiveAlbum(const std::string &album) {
  for (const char *keyword : kCompilationKeywords) {
    if (StrUtils::ContainsInsensitive(album, keyword)) {
      return true;
    }
  }
  return false;
}

void AlbumCoverFetcherSearch::ScoreResults(const CoverSearchRequest &request, float provider_quality, const std::string &provider_name,
                                           CoverProviderSearchResults *results) {
  if (!results) {
    return;
  }
  for (CoverProviderSearchResult &result : *results) {
    result.provider = provider_name;
    result.score_provider = provider_quality;
    result.score_match = 0.0f;
    if (StrUtils::ToLower(result.artist) == StrUtils::ToLower(request.artist)) {
      result.score_match += 0.5f;
    }
    if (StrUtils::ToLower(result.album) == StrUtils::ToLower(request.album)) {
      result.score_match += 0.5f;
    }
    if (StrUtils::ToLower(result.artist) != StrUtils::ToLower(request.artist) &&
        StrUtils::ToLower(result.album) != StrUtils::ToLower(request.album)) {
      result.score_match -= 1.5f;
    }
    if (request.album.empty() && StrUtils::ToLower(result.artist) != StrUtils::ToLower(request.artist)) {
      result.score_match -= 1.0f;
    }
    if (request.album.empty() && IsCompilationOrLiveAlbum(result.album)) {
      result.score_match -= 1.0f;
    } else if (request.album.empty() && StrUtils::ContainsInsensitive(result.album, "soundtrack")) {
      result.score_match -= 0.5f;
    }
    result.score_quality += ScoreImage(result.image_width, result.image_height);
  }
}

bool AlbumCoverFetcherSearch::CompareScore(const CoverProviderSearchResult &a, const CoverProviderSearchResult &b) {
  return a.score() > b.score();
}

bool AlbumCoverFetcherSearch::CompareNumber(const CoverProviderSearchResult &a, const CoverProviderSearchResult &b) {
  return a.number < b.number;
}

void AlbumCoverFetcherSearch::SortByScore(CoverProviderSearchResults *results) {
  if (!results) {
    return;
  }
  std::stable_sort(results->begin(), results->end(), CompareScore);
}
