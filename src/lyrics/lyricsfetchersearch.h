#ifndef STRAWBERRY_LYRICSFETCHERSEARCH_H
#define STRAWBERRY_LYRICSFETCHERSEARCH_H

#include "core/signal.h"
#include "lyrics/lyricssearchrequest.h"
#include "lyrics/lyricssearchresult.h"

#include <cstdint>
#include <string>

class LyricsProviders;

class LyricsFetcherSearch {
 public:
  LyricsFetcherSearch(uint64_t id, LyricsSearchRequest request, LyricsProviders *providers);
  void Start();
  uint64_t id() const { return id_; }
  const LyricsSearchResults &results() const { return results_; }

  Signal<uint64_t, std::string, std::string> LyricsFetched;
  Signal<uint64_t, LyricsSearchResults> SearchFinished;

 private:
  void FetchFromIndex(size_t index);

  uint64_t id_ = 0;
  LyricsSearchRequest request_;
  LyricsProviders *providers_ = nullptr;
  LyricsSearchResults results_;
};

#endif
