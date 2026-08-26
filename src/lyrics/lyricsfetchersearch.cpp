#include "lyrics/lyricsfetchersearch.h"

#include "lyrics/lyricsproviders.h"

LyricsFetcherSearch::LyricsFetcherSearch(uint64_t id, LyricsSearchRequest request, LyricsProviders *providers)
    : id_(id), request_(std::move(request)), providers_(providers) {}

void LyricsFetcherSearch::Start() { FetchFromIndex(0); }

void LyricsFetcherSearch::FetchFromIndex(size_t index) {
  if (!providers_) {
    SearchFinished.Emit(id_, results_);
    return;
  }
  const auto all = providers_->All();
  while (index < all.size() && !all[index]->enabled()) {
    ++index;
  }
  if (index >= all.size()) {
    SearchFinished.Emit(id_, results_);
    return;
  }
  all[index]->StartSearch(static_cast<int>(id_), request_, providers_->network(), [this, index](int, const LyricsSearchResults &found) {
    if (!found.empty() && !found.front().lyrics.empty()) {
      results_.insert(results_.end(), found.begin(), found.end());
      LyricsFetched.Emit(id_, found.front().provider, found.front().lyrics);
      SearchFinished.Emit(id_, results_);
      return;
    }
    FetchFromIndex(index + 1);
  });
}
