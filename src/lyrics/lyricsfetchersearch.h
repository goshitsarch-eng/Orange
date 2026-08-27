#ifndef STRAWBERRY_LYRICSFETCHERSEARCH_H
#define STRAWBERRY_LYRICSFETCHERSEARCH_H

#include "core/signal.h"
#include "lyrics/lyricssearchrequest.h"
#include "lyrics/lyricssearchresult.h"

#include <glib.h>

#include <cstdint>
#include <string>

class LyricsProviders;

class LyricsFetcherSearch {
 public:
  LyricsFetcherSearch(uint64_t id, LyricsSearchRequest request, LyricsProviders *providers);
  ~LyricsFetcherSearch();
  void Start();
  void HandleTimeout(bool hard);
  uint64_t id() const { return id_; }
  const LyricsSearchResults &results() const { return results_; }

  Signal<uint64_t, std::string, std::string> LyricsFetched;
  Signal<uint64_t, LyricsSearchResults> SearchFinished;

 private:
  void OnProviderFinished(const LyricsSearchResults &found);
  void Finish();
  void CancelTimeouts();
  int ElapsedMs() const;

  uint64_t id_ = 0;
  LyricsSearchRequest request_;
  LyricsProviders *providers_ = nullptr;
  LyricsSearchResults results_;
  int pending_ = 0;
  bool finished_ = false;
  float best_score_ = 0.0f;
  gint64 started_us_ = 0;
  guint early_timeout_id_ = 0;
  guint hard_timeout_id_ = 0;
};

#endif
