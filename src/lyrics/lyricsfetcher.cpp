#include "lyrics/lyricsfetcher.h"

#include "lyrics/lyricsfetchersearch.h"

LyricsFetcher::LyricsFetcher(LyricsProviders *lyrics_providers) : lyrics_providers_(lyrics_providers) {}

uint64_t LyricsFetcher::Search(const std::string &effective_albumartist, const std::string &artist, const std::string &album,
                               const std::string &title, int64_t duration) {
  LyricsSearchRequest request;
  request.albumartist = effective_albumartist;
  request.artist = artist;
  request.album = album;
  request.title = title;
  request.duration = duration;
  const uint64_t id = next_id_++;
  queued_.emplace(id, request);
  StartNext();
  return id;
}

void LyricsFetcher::Clear() {
  while (!queued_.empty()) {
    queued_.pop();
  }
  for (auto &entry : active_) {
    delete entry.second;
  }
  active_.clear();
}

void LyricsFetcher::StartNext() {
  while (active_.size() < 2 && !queued_.empty()) {
    auto item = queued_.front();
    queued_.pop();
    auto *search = new LyricsFetcherSearch(item.first, item.second, lyrics_providers_);
    search->LyricsFetched.Connect([this](uint64_t id, const std::string &provider, const std::string &lyrics) {
      LyricsFetched.Emit(id, provider, lyrics);
    });
    search->SearchFinished.Connect([this](uint64_t id, const LyricsSearchResults &results) {
      SearchFinished.Emit(id, results);
      auto it = active_.find(id);
      if (it != active_.end()) {
        delete it->second;
        active_.erase(it);
      }
      StartNext();
    });
    active_[item.first] = search;
    search->Start();
  }
}
