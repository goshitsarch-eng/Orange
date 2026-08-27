#ifndef STRAWBERRY_LYRICSSEARCHSCORE_H
#define STRAWBERRY_LYRICSSEARCHSCORE_H

#include "lyrics/lyricssearchrequest.h"
#include "lyrics/lyricssearchresult.h"
#include "utilities/strutils.h"

#include <algorithm>

namespace LyricsSearchScore {

constexpr float kHighScore = 2.5f;

inline float Score(const LyricsSearchRequest &request, const LyricsSearchResult &result) {
  if (result.lyrics.empty()) {
    return 0.0f;
  }
  float score = 1.0f;
  if (!request.title.empty() && StrUtils::ContainsInsensitive(result.lyrics, request.title)) {
    score += 1.0f;
  }
  const std::string artist = request.artist.empty() ? request.albumartist : request.artist;
  if (!artist.empty() && StrUtils::ContainsInsensitive(result.lyrics, artist)) {
    score += 0.5f;
  }
  if (!request.album.empty() && StrUtils::ContainsInsensitive(result.lyrics, request.album)) {
    score += 0.25f;
  }
  return score;
}

inline float BestScore(const LyricsSearchResults &results) {
  float best = 0.0f;
  for (const LyricsSearchResult &result : results) {
    best = std::max(best, result.score);
  }
  return best;
}

inline bool ShouldFinishEarly(float best_score) { return best_score >= kHighScore; }

}  // namespace LyricsSearchScore

#endif  // STRAWBERRY_LYRICSSEARCHSCORE_H
