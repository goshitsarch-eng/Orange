#include "streaming/streamingcoverdownload.h"

#include "covermanager/coverutils.h"
#include "streaming/streamingsearchopts.h"

#include <memory>

namespace StreamingCoverDownload {

void AfterList(NetworkAccessManager *network, const std::map<std::string, std::string> &headers, const std::string &group, SongList songs,
               StreamingService::SearchCallback done, std::function<void(const std::string &)> status,
               StreamingPage::ProgressCallback progress, StreamingPage::StillCurrent still_current) {
  songs = StreamingSearchOpts::Finish(songs, group);
  if (!network || !Enabled(group)) {
    if (done) {
      done(songs);
    }
    return;
  }
  const std::vector<Job> jobs = UniqueAlbums(songs);
  if (jobs.empty()) {
    if (done) {
      done(songs);
    }
    return;
  }
  struct State {
    NetworkAccessManager *network = nullptr;
    std::map<std::string, std::string> headers;
    SongList songs;
    StreamingService::SearchCallback done;
    std::function<void(const std::string &)> status;
    StreamingPage::ProgressCallback progress;
    StreamingPage::StillCurrent still_current;
    std::vector<Job> jobs;
    size_t index = 0;
  };
  auto state = std::make_shared<State>();
  state->network = network;
  state->headers = headers;
  state->songs = std::move(songs);
  state->done = std::move(done);
  state->status = std::move(status);
  state->progress = std::move(progress);
  state->still_current = std::move(still_current);
  state->jobs = jobs;
  if (state->status) {
    state->status(Receiving(static_cast<int>(state->jobs.size())));
  }
  if (state->progress) {
    state->progress(0, static_cast<int>(state->jobs.size()));
  }
  auto step = std::make_shared<std::function<void()>>();
  *step = [state, step]() {
    if (state->still_current && !state->still_current()) {
      return;
    }
    if (state->index >= state->jobs.size()) {
      if (state->done) {
        state->done(state->songs);
      }
      return;
    }
    const Job job = state->jobs[state->index++];
    const std::string path = CachePath(job.filename);
    auto finish_job = [state, step, job, path]() {
      if (CacheReady(path)) {
        ApplyLocalCover(state->songs, job.album_id, path);
      }
      if (state->progress) {
        state->progress(static_cast<int>(state->index), static_cast<int>(state->jobs.size()));
      }
      (*step)();
    };
    if (CacheReady(path)) {
      finish_job();
      return;
    }
    if (job.url.empty()) {
      finish_job();
      return;
    }
    state->network->Get(
        job.url,
        [state, finish_job, path](const NetworkAccessManager::Response &response) {
          if (state->still_current && !state->still_current()) {
            return;
          }
          if (response.ok() && CoverUtils::LooksLikeImage(response.body)) {
            FileUtils::WriteFile(path, response.body);
          }
          finish_job();
        },
        state->headers);
  };
  (*step)();
}

}  // namespace StreamingCoverDownload
