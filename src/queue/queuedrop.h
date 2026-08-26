#ifndef STRAWBERRY_QUEUEDROP_H
#define STRAWBERRY_QUEUEDROP_H

#include "utilities/strutils.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace QueueDrop {

inline constexpr const char *kQueueRowsPrefix = "strawberry-queue-rows:";
inline constexpr const char *kPlaylistRowsPrefix = "strawberry-playlist-rows:";

inline bool HasPrefix(const std::string &text, const char *prefix) { return prefix && text.compare(0, std::strlen(prefix), prefix) == 0; }

inline bool IsQueueRows(const std::string &text) { return HasPrefix(text, kQueueRowsPrefix); }

inline bool IsPlaylistRows(const std::string &text) { return HasPrefix(text, kPlaylistRowsPrefix); }

inline std::vector<int> ParseRows(const std::string &text, const char *prefix) {
  std::vector<int> rows;
  if (!HasPrefix(text, prefix)) {
    return rows;
  }
  for (const std::string &part : StrUtils::Split(text.substr(std::strlen(prefix)), ',')) {
    if (!part.empty()) {
      rows.push_back(std::atoi(part.c_str()));
    }
  }
  return rows;
}

inline std::string RowsPayload(const std::vector<int> &rows, const char *prefix) {
  std::string text = prefix ? prefix : "";
  for (size_t i = 0; i < rows.size(); ++i) {
    if (i) {
      text += ",";
    }
    text += std::to_string(rows[i]);
  }
  return text;
}

inline std::vector<std::string> ParseUrls(const std::string &text) {
  std::vector<std::string> urls;
  if (IsQueueRows(text) || IsPlaylistRows(text)) {
    return urls;
  }
  for (std::string part : StrUtils::Split(text, '\n')) {
    if (!part.empty() && part.back() == '\r') {
      part.pop_back();
    }
    if (!part.empty()) {
      urls.push_back(part);
    }
  }
  return urls;
}

inline int DestinationAfterRemove(int to, const std::vector<int> &from) {
  int dest = to;
  for (int idx : from) {
    if (idx < to) {
      --dest;
    }
  }
  return dest;
}

}  // namespace QueueDrop

#endif
