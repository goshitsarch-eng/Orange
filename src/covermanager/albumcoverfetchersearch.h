#ifndef STRAWBERRY_ALBUMCOVERFETCHERSEARCH_H
#define STRAWBERRY_ALBUMCOVERFETCHERSEARCH_H

#include "covermanager/albumcoverfetcher.h"

class AlbumCoverFetcherSearch {
 public:
  static constexpr float kTargetSize = 500.0f;
  static constexpr float kGoodScore = 4.0f;

  static float ScoreImage(int width, int height);
  static void ScoreResults(const CoverSearchRequest &request, float provider_quality, const std::string &provider_name,
                           CoverProviderSearchResults *results);
  static bool CompareScore(const CoverProviderSearchResult &a, const CoverProviderSearchResult &b);
  static bool CompareNumber(const CoverProviderSearchResult &a, const CoverProviderSearchResult &b);
  static void SortByScore(CoverProviderSearchResults *results);

  static bool IsCompilationOrLiveAlbum(const std::string &album);
};

#endif
