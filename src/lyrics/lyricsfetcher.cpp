#include "lyrics/lyricsfetcher.h"

#include "core/song.h"
#include "lyrics/lyricsfetcherpacing.h"
#include "lyrics/lyricsfetchersearch.h"

namespace {

gboolean LyricsFetcherStarterCb(gpointer data) {
  auto *self = static_cast<LyricsFetcher *>(data);
  return self->StartNext() ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

}  // namespace

LyricsFetcher::LyricsFetcher(LyricsProviders *lyrics_providers) : lyrics_providers_(lyrics_providers) {}

LyricsFetcher::~LyricsFetcher() {
  CancelStarter();
  Clear();
}

uint64_t LyricsFetcher::Search(const std::string &effective_albumartist, const std::string &artist, const std::string &album,
                               const std::string &title, int64_t duration) {
  LyricsSearchRequest request;
  request.albumartist = effective_albumartist;
  request.artist = artist;
  request.album = album;
  request.title = title;
  request.duration = duration;
  request = LyricsFetcherPacing::Normalize(request);
  const uint64_t id = next_id_++;
  queued_.emplace(id, request);
  if (!starter_id_) {
    starter_id_ = g_timeout_add(LyricsFetcherPacing::kStarterDelayMs, LyricsFetcherStarterCb, this);
  }
  if (LyricsFetcherPacing::CanStartMore(static_cast<int>(active_.size()))) {
    StartNext();
  }
  return id;
}

void LyricsFetcher::Clear() {
  CancelStarter();
  while (!queued_.empty()) {
    queued_.pop();
  }
  for (auto &entry : active_) {
    delete entry.second;
  }
  active_.clear();
}

bool LyricsFetcher::StartNext() {
  while (!queued_.empty() && LyricsFetcherPacing::CanStartMore(static_cast<int>(active_.size()))) {
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
      if (!starter_id_ && !queued_.empty()) {
        starter_id_ = g_timeout_add(LyricsFetcherPacing::kStarterDelayMs, LyricsFetcherStarterCb, this);
      }
    });
    active_[item.first] = search;
    search->Start();
  }
  if (queued_.empty()) {
    starter_id_ = 0;
    return false;
  }
  return true;
}

void LyricsFetcher::CancelStarter() {
  if (starter_id_) {
    g_source_remove(starter_id_);
    starter_id_ = 0;
  }
}
