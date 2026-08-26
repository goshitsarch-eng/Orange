#include "streaming/streamingpage.h"

#include "streaming/streamingabort.h"

#include <memory>

namespace StreamingPage {

void GetAll(NetworkAccessManager *network, UrlForOffset url_for, const std::map<std::string, std::string> &headers, ParsePage parse,
            DoneCallback callback, ProgressCallback progress, StillCurrent still_current, int limit, int max_items, ErrorCallback error) {
  if (!network || !url_for || !parse) {
    if (error) {
      error(StreamingAbort::HttpError(0, {}));
    }
    if (callback) {
      callback({});
    }
    return;
  }
  struct State {
    NetworkAccessManager *network = nullptr;
    UrlForOffset url_for;
    std::map<std::string, std::string> headers;
    ParsePage parse;
    DoneCallback callback;
    ProgressCallback progress;
    StillCurrent still_current;
    ErrorCallback error;
    int limit = kDefaultLimit;
    int max_items = 0;
    int offset = 0;
    int pages = 0;
    std::string next_url;
    SongList songs;
  };
  auto state = std::make_shared<State>();
  state->network = network;
  state->url_for = std::move(url_for);
  state->headers = headers;
  state->parse = std::move(parse);
  state->callback = std::move(callback);
  state->progress = std::move(progress);
  state->still_current = std::move(still_current);
  state->error = std::move(error);
  state->limit = limit > 0 ? limit : kDefaultLimit;
  state->max_items = max_items;

  auto fetch = std::make_shared<std::function<void()>>();
  *fetch = [state, fetch]() {
    if (state->still_current && !state->still_current()) {
      return;
    }
    if (state->pages >= kMaxPages) {
      if (state->callback) {
        state->callback(state->songs);
      }
      return;
    }
    const int page_limit = PageLimit(state->limit, state->max_items, static_cast<int>(state->songs.size()));
    if (page_limit <= 0) {
      if (state->callback) {
        state->callback(state->songs);
      }
      return;
    }
    const std::string url = state->next_url.empty() ? state->url_for(state->offset, page_limit) : state->next_url;
    state->next_url.clear();
    if (url.empty()) {
      if (state->callback) {
        state->callback(state->songs);
      }
      return;
    }
    const int requested_offset = state->offset;
    ++state->pages;
    state->network->Get(
        url,
        [state, fetch, requested_offset](const NetworkAccessManager::Response &response) {
          if (state->still_current && !state->still_current()) {
            return;
          }
          Page page;
          if (response.ok()) {
            page = state->parse(response.body, requested_offset, state->limit);
          } else if (state->songs.empty() && state->error) {
            state->error(StreamingAbort::HttpError(response.status, response.error));
            if (state->callback) {
              state->callback({});
            }
            return;
          }
          state->songs.insert(state->songs.end(), page.songs.begin(), page.songs.end());
          if (ReachedMax(static_cast<int>(state->songs.size()), state->max_items)) {
            state->songs.resize(static_cast<size_t>(state->max_items));
          }
          if (state->progress) {
            state->progress(static_cast<int>(state->songs.size()), page.total);
          }
          const int next = NextOffset(page);
          if (!ReachedMax(static_cast<int>(state->songs.size()), state->max_items) && NeedAnotherPage(page) && next > requested_offset &&
              state->pages < kMaxPages) {
            state->offset = next;
            state->next_url = page.next_url;
            (*fetch)();
            return;
          }
          if (state->callback) {
            state->callback(state->songs);
          }
        },
        state->headers);
  };
  (*fetch)();
}

}  // namespace StreamingPage
