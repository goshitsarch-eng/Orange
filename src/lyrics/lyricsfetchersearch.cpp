#include "lyrics/lyricsfetchersearch.h"

#include "lyrics/lyricsproviders.h"
#include "lyrics/lyricssearchscore.h"

#include <algorithm>
#include <vector>

LyricsFetcherSearch::LyricsFetcherSearch(uint64_t id, LyricsSearchRequest request, LyricsProviders *providers)
    : id_(id), request_(std::move(request)), providers_(providers) {}

void LyricsFetcherSearch::Start() {
  if (!providers_) {
    SearchFinished.Emit(id_, results_);
    return;
  }
  pending_ = 0;
  finished_ = false;
  best_score_ = 0.0f;
  std::vector<LyricsProvider *> enabled;
  for (LyricsProvider *provider : providers_->All()) {
    if (provider && provider->enabled()) {
      enabled.push_back(provider);
    }
  }
  if (enabled.empty()) {
    SearchFinished.Emit(id_, results_);
    return;
  }
  pending_ = static_cast<int>(enabled.size());
  for (LyricsProvider *provider : enabled) {
    provider->StartSearch(static_cast<int>(id_), request_, providers_->network(),
                          [this](int, const LyricsSearchResults &found) { OnProviderFinished(found); });
  }
}

void LyricsFetcherSearch::OnProviderFinished(const LyricsSearchResults &found) {
  if (finished_) {
    return;
  }
  for (LyricsSearchResult result : found) {
    if (result.lyrics.empty()) {
      continue;
    }
    const float scored = LyricsSearchScore::Score(request_, result);
    result.score = std::max(result.score, scored);
    results_.push_back(result);
    if (result.score > best_score_) {
      best_score_ = result.score;
      LyricsFetched.Emit(id_, result.provider, result.lyrics);
    }
  }
  --pending_;
  if (pending_ <= 0 || LyricsSearchScore::ShouldFinishEarly(best_score_)) {
    Finish();
  }
}

void LyricsFetcherSearch::Finish() {
  if (finished_) {
    return;
  }
  finished_ = true;
  SearchFinished.Emit(id_, results_);
}
