#ifndef STRAWBERRY_STREAMINGPAGE_H
#define STRAWBERRY_STREAMINGPAGE_H

#include "core/network.h"
#include "core/song.h"
#include "utilities/jsonutils.h"

#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace StreamingPage {

constexpr int kDefaultLimit = 50;
constexpr int kMaxPages = 100;

struct Page {
  SongList songs;
  int offset = 0;
  int limit = 0;
  int total = -1;
  std::string next_url;
};

using UrlForOffset = std::function<std::string(int offset, int limit)>;
using ParsePage = std::function<Page(const std::string &json, int offset, int limit)>;
using ProgressCallback = std::function<void(int received, int total)>;
using StillCurrent = std::function<bool()>;
using DoneCallback = std::function<void(const SongList &)>;

inline std::string FirstPresentString(const std::string &json, const std::vector<std::vector<std::string>> &paths) {
  for (const auto &path : paths) {
    const std::string value = JsonUtils::GetString(json, path);
    if (!value.empty() && value != "null") {
      return value;
    }
  }
  return {};
}

inline int FirstPresentInt(const std::string &json, const std::vector<std::vector<std::string>> &paths, int fallback) {
  for (const auto &path : paths) {
    const std::string text = JsonUtils::GetString(json, path);
    if (!text.empty()) {
      return static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
    }
  }
  return fallback;
}

inline Page ParseMeta(const std::string &json, int requested_offset, int requested_limit) {
  Page page;
  page.offset = FirstPresentInt(json, {{"offset"}, {"artists", "offset"}, {"albums", "offset"}, {"tracks", "offset"}}, requested_offset);
  page.limit = FirstPresentInt(json, {{"limit"}, {"artists", "limit"}, {"albums", "limit"}, {"tracks", "limit"}}, requested_limit);
  page.total = FirstPresentInt(json,
                               {{"totalNumberOfItems"}, {"total"}, {"artists", "total"}, {"albums", "total"}, {"tracks", "total"}}, -1);
  page.next_url = FirstPresentString(json, {{"next"}, {"artists", "next"}, {"albums", "next"}, {"tracks", "next"}});
  return page;
}

inline int NextOffset(const Page &page) { return page.offset + static_cast<int>(page.songs.size()); }

inline bool NeedAnotherPage(const Page &page) {
  if (page.songs.empty()) {
    return false;
  }
  if (!page.next_url.empty()) {
    return true;
  }
  if (page.total >= 0) {
    return NextOffset(page) < page.total;
  }
  return page.limit > 0 && static_cast<int>(page.songs.size()) >= page.limit;
}

void GetAll(NetworkAccessManager *network, UrlForOffset url_for, const std::map<std::string, std::string> &headers, ParsePage parse,
            DoneCallback callback, ProgressCallback progress = {}, StillCurrent still_current = {}, int limit = kDefaultLimit);

}  // namespace StreamingPage

#endif
