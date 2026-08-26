#ifndef STRAWBERRY_LYRICSFETCHER_H
#define STRAWBERRY_LYRICSFETCHER_H

#include "core/signal.h"
#include "lyrics/lyricssearchrequest.h"
#include "lyrics/lyricssearchresult.h"

#include <cstdint>
#include <map>
#include <queue>
#include <string>

class LyricsFetcherSearch;
class LyricsProviders;

class LyricsFetcher {
 public:
  explicit LyricsFetcher(LyricsProviders *lyrics_providers);

  uint64_t Search(const std::string &effective_albumartist, const std::string &artist, const std::string &album,
                  const std::string &title, int64_t duration = -1);
  void Clear();

  Signal<uint64_t, std::string, std::string> LyricsFetched;
  Signal<uint64_t, LyricsSearchResults> SearchFinished;

 private:
  void StartNext();

  LyricsProviders *lyrics_providers_ = nullptr;
  uint64_t next_id_ = 1;
  std::queue<std::pair<uint64_t, LyricsSearchRequest>> queued_;
  std::map<uint64_t, LyricsFetcherSearch *> active_;
};

#endif
