#ifndef STRAWBERRY_LYRICSFETCHER_H
#define STRAWBERRY_LYRICSFETCHER_H

#include "core/signal.h"
#include "lyrics/lyricssearchrequest.h"
#include "lyrics/lyricssearchresult.h"

#include <glib.h>

#include <cstdint>
#include <map>
#include <queue>
#include <string>
#include <vector>

class LyricsFetcherSearch;
class LyricsProviders;

class LyricsFetcher {
 public:
  explicit LyricsFetcher(LyricsProviders *lyrics_providers);
  ~LyricsFetcher();

  uint64_t Search(const std::string &effective_albumartist, const std::string &artist, const std::string &album,
                  const std::string &title, int64_t duration = -1);
  void Clear();
  void StartNext();
  // Called from the GLib sources below; not part of the public surface in spirit.
  bool OnStarterTick();
  void ReapFinished();

  Signal<uint64_t, std::string, std::string> LyricsFetched;
  Signal<uint64_t, LyricsSearchResults> SearchFinished;

 private:
  void CancelStarter();
  void EnsureStarter();

  LyricsProviders *lyrics_providers_ = nullptr;
  uint64_t next_id_ = 1;
  std::queue<std::pair<uint64_t, LyricsSearchRequest>> queued_;
  std::map<uint64_t, LyricsFetcherSearch *> active_;
  // Searches that have finished and are waiting to be deleted once their own call stack has unwound.
  std::vector<LyricsFetcherSearch *> finished_;
  guint starter_id_ = 0;
  guint reap_id_ = 0;
};

#endif
