#include "lyrics/lyricsfetchersearch.h"

#include "lyrics/lyricsproviders.h"
#include "lyrics/lyricssearchscore.h"
#include "lyrics/lyricssearchtimeout.h"

#include <algorithm>
#include <vector>

namespace {

gboolean LyricsSearchEarlyTimeoutCb(gpointer data) {
  static_cast<LyricsFetcherSearch *>(data)->HandleTimeout(false);
  return G_SOURCE_REMOVE;
}

gboolean LyricsSearchHardTimeoutCb(gpointer data) {
  static_cast<LyricsFetcherSearch *>(data)->HandleTimeout(true);
  return G_SOURCE_REMOVE;
}

}  // namespace

LyricsFetcherSearch::LyricsFetcherSearch(uint64_t id, LyricsSearchRequest request, LyricsProviders *providers)
    : id_(id), request_(std::move(request)), providers_(providers) {}

LyricsFetcherSearch::~LyricsFetcherSearch() { CancelTimeouts(); }

void LyricsFetcherSearch::Start() {
  if (!providers_ || LyricsSearchTimeout::ShouldSkipCommercial(request_.artist, request_.title)) {
    SearchFinished.Emit(id_, results_);
    return;
  }
  pending_ = 0;
  finished_ = false;
  best_score_ = 0.0f;
  started_us_ = g_get_monotonic_time();
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
  early_timeout_id_ = g_timeout_add(LyricsSearchTimeout::kEarlyMs, LyricsSearchEarlyTimeoutCb, this);
  hard_timeout_id_ = g_timeout_add(LyricsSearchTimeout::kHardMs, LyricsSearchHardTimeoutCb, this);
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

void LyricsFetcherSearch::HandleTimeout(bool hard) {
  if (hard) {
    hard_timeout_id_ = 0;
    Finish();
    return;
  }
  early_timeout_id_ = 0;
  if (LyricsSearchTimeout::ShouldFinishEarly(ElapsedMs(), !results_.empty())) {
    Finish();
  }
}

void LyricsFetcherSearch::Finish() {
  if (finished_) {
    return;
  }
  finished_ = true;
  CancelTimeouts();
  SearchFinished.Emit(id_, results_);
}

void LyricsFetcherSearch::CancelTimeouts() {
  if (early_timeout_id_) {
    g_source_remove(early_timeout_id_);
    early_timeout_id_ = 0;
  }
  if (hard_timeout_id_) {
    g_source_remove(hard_timeout_id_);
    hard_timeout_id_ = 0;
  }
}

int LyricsFetcherSearch::ElapsedMs() const {
  if (started_us_ <= 0) {
    return 0;
  }
  return static_cast<int>((g_get_monotonic_time() - started_us_) / 1000);
}
