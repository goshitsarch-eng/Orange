#ifndef STRAWBERRY_ALBUMCOVERFETCHERSEARCH_H
#define STRAWBERRY_ALBUMCOVERFETCHERSEARCH_H

#include "core/song.h"
#include "covermanager/albumcoverfetcher.h"
#include "covermanager/coverprovider.h"
#include "utilities/strutils.h"

#include <glib.h>

class AlbumCoverFetcherSearch {
 public:
  static constexpr float kTargetSize = 500.0f;
  static constexpr float kGoodScore = 4.0f;
  static constexpr int kMaxConcurrentRequests = 5;

  static float ScoreImage(int width, int height);
  static void ScoreResults(const CoverSearchRequest &request, float provider_quality, const std::string &provider_name,
                           CoverProviderSearchResults *results);
  static bool CompareScore(const CoverProviderSearchResult &a, const CoverProviderSearchResult &b);
  static bool CompareNumber(const CoverProviderSearchResult &a, const CoverProviderSearchResult &b);
  static void SortByScore(CoverProviderSearchResults *results);

  static bool IsCompilationOrLiveAlbum(const std::string &album);

  static bool ShouldUseProvider(bool enabled, bool authentication_required, bool authenticated, bool batch_supported,
                                bool allow_missing_album, const CoverSearchRequest &request) {
    if (!enabled) {
      return false;
    }
    if (authentication_required && !authenticated) {
      return false;
    }
    if (request.batch && !batch_supported) {
      return false;
    }
    if (!allow_missing_album && request.album.empty() && !request.title.empty()) {
      return false;
    }
    return true;
  }

  static bool ShouldUseProvider(const CoverProvider *provider, const CoverSearchRequest &request) {
    if (!provider) {
      return false;
    }
    return ShouldUseProvider(provider->enabled(), provider->authentication_required(), provider->authenticated(), provider->batch(),
                             provider->allow_missing_album(), request);
  }

  static bool ShouldTerminateSearch(const CoverSearchRequest &request) {
    return StrUtils::ToLower(request.artist) == "commercial-free" && StrUtils::ToLower(request.title) == "listener-supported";
  }

  static CoverSearchRequest RequestFromSong(const Song &song, bool search = false, bool batch = false) {
    return MakeRequest(0, song.EffectiveAlbumartist(), song.album(), song.title(), search, batch);
  }

  static CoverSearchRequest MakeRequest(uint64_t id, const std::string &artist, const std::string &album, const std::string &title, bool search,
                                        bool batch) {
    CoverSearchRequest request;
    request.id = id;
    request.artist = artist;
    request.album = Song::AlbumRemoveDiscMisc(album);
    request.title = title;
    request.search = search;
    request.batch = batch;
    return request;
  }

  static Song SongFromRequest(const CoverSearchRequest &request) {
    Song song;
    song.set_artist(request.artist);
    song.set_albumartist(request.artist);
    song.set_album(request.album);
    song.set_title(request.title);
    return song;
  }

  static CoverProviderSearchResult FromHit(const std::string &provider, const std::string &artist, const std::string &album,
                                           const std::string &image_url, int width = 0, int height = 0, const std::string &image_data = {}) {
    CoverProviderSearchResult result;
    result.provider = provider;
    result.artist = artist;
    result.album = album;
    result.image_url = image_url;
    result.image_data = image_data;
    result.image_width = width;
    result.image_height = height;
    return result;
  }

  static void AssignNumbers(CoverProviderSearchResults *results) {
    if (!results) {
      return;
    }
    for (size_t i = 0; i < results->size(); ++i) {
      (*results)[i].number = static_cast<int>(i + 1);
    }
  }

  static const CoverProviderSearchResult *Best(const CoverProviderSearchResults &results) {
    if (results.empty()) {
      return nullptr;
    }
    const CoverProviderSearchResult *best = &results.front();
    for (const CoverProviderSearchResult &result : results) {
      if (result.score() > best->score()) {
        best = &result;
      }
    }
    return best;
  }

  static bool IsHttpUrl(const std::string &value) {
    return StrUtils::StartsWith(value, "http://") || StrUtils::StartsWith(value, "https://");
  }

  static std::string ResultLabel(const CoverProviderSearchResult &result) {
    if (!result.artist.empty() && !result.album.empty()) {
      return result.artist + " - " + result.album;
    }
    if (!result.album.empty()) {
      return result.album;
    }
    if (!result.artist.empty()) {
      return result.artist;
    }
    return result.provider;
  }

  static std::string ResultSubtitle(const CoverProviderSearchResult &result) {
    std::string text = result.provider;
    if (result.image_width > 0 && result.image_height > 0) {
      text += " · " + std::to_string(result.image_width) + "×" + std::to_string(result.image_height);
    }
    char score[32];
    g_snprintf(score, sizeof(score), " · %.1f", result.score());
    text += score;
    return text;
  }

  static std::string StatusSearching(const std::string &album) {
    return album.empty() ? "Searching providers…" : "Searching providers for “" + album + "”…";
  }

  static std::string StatusFound(int count) {
    if (count <= 0) {
      return "No covers found";
    }
    return std::to_string(count) + (count == 1 ? " cover found" : " covers found");
  }
};

#endif
