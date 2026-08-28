#include "lyrics/lyricsfetcher.h"

#include "core/song.h"
#include "lyrics/lyricsfetcherpacing.h"
#include "lyrics/lyricsfetchersearch.h"

namespace {

gboolean LyricsFetcherStarterCb(gpointer data) {
  auto *self = static_cast<LyricsFetcher *>(data);
  return self->OnStarterTick() ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

gboolean LyricsFetcherReapCb(gpointer data) {
  static_cast<LyricsFetcher *>(data)->ReapFinished();
  return G_SOURCE_REMOVE;
}

}  // namespace

LyricsFetcher::LyricsFetcher(LyricsProviders *lyrics_providers) : lyrics_providers_(lyrics_providers) {}

LyricsFetcher::~LyricsFetcher() {
  CancelStarter();
  if (reap_id_) {
    g_source_remove(reap_id_);
    reap_id_ = 0;
  }
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
  EnsureStarter();
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
  ReapFinished();
}

void LyricsFetcher::StartNext() {
  while (!queued_.empty() && LyricsFetcherPacing::CanStartMore(static_cast<int>(active_.size()))) {
    auto item = queued_.front();
    queued_.pop();
    auto *search = new LyricsFetcherSearch(item.first, item.second, lyrics_providers_);
    search->LyricsFetched.Connect([this](uint64_t id, const std::string &provider, const std::string &lyrics) {
      LyricsFetched.Emit(id, provider, lyrics);
    });
    search->SearchFinished.Connect([this](uint64_t id, const LyricsSearchResults &results) {
      // This runs from inside the search's own Finish(), so the search is still on the stack and must not
      // be destroyed here. Take it out of the active set and let an idle callback delete it once the stack
      // has unwound.
      auto it = active_.find(id);
      if (it != active_.end()) {
        finished_.push_back(it->second);
        active_.erase(it);
        if (!reap_id_) {
          reap_id_ = g_idle_add(LyricsFetcherReapCb, this);
        }
      }
      SearchFinished.Emit(id, results);
      EnsureStarter();
    });
    active_[item.first] = search;
    search->Start();
  }
}

bool LyricsFetcher::OnStarterTick() {
  StartNext();
  if (queued_.empty()) {
    // Returning false makes GLib drop the source, so forget the id rather than trying to remove it later.
    starter_id_ = 0;
    return false;
  }
  return true;
}

void LyricsFetcher::EnsureStarter() {
  if (!starter_id_ && !queued_.empty()) {
    starter_id_ = g_timeout_add(LyricsFetcherPacing::kStarterDelayMs, LyricsFetcherStarterCb, this);
  }
}

void LyricsFetcher::ReapFinished() {
  reap_id_ = 0;
  // Deleting a search can start another one, so work off a copy rather than the member.
  std::vector<LyricsFetcherSearch *> searches;
  searches.swap(finished_);
  for (LyricsFetcherSearch *search : searches) {
    delete search;
  }
}

void LyricsFetcher::CancelStarter() {
  if (starter_id_) {
    g_source_remove(starter_id_);
    starter_id_ = 0;
  }
}
