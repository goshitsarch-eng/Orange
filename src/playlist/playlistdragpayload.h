#ifndef STRAWBERRY_PLAYLISTDRAGPAYLOAD_H
#define STRAWBERRY_PLAYLISTDRAGPAYLOAD_H

#include "queue/queuedrop.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace PlaylistDragPayload {

inline constexpr const char *kPrefix = QueueDrop::kPlaylistRowsPrefix;

struct Payload {
  int source_id = -1;
  std::vector<int> rows;
};

inline bool IsPlaylistRows(const std::string &text) { return QueueDrop::IsPlaylistRows(text); }

inline std::string Encode(int source_id, const std::vector<int> &rows) {
  std::string text = kPrefix;
  if (source_id >= 0) {
    text += std::to_string(source_id) + "|";
  }
  for (size_t i = 0; i < rows.size(); ++i) {
    if (i) {
      text += ",";
    }
    text += std::to_string(rows[i]);
  }
  return text;
}

inline Payload Decode(const std::string &text) {
  Payload payload;
  if (!IsPlaylistRows(text)) {
    return payload;
  }
  std::string rest = text.substr(std::strlen(kPrefix));
  const auto bar = rest.find('|');
  if (bar != std::string::npos) {
    payload.source_id = std::atoi(rest.substr(0, bar).c_str());
    rest = rest.substr(bar + 1);
  }
  for (const std::string &part : StrUtils::Split(rest, ',')) {
    if (!part.empty()) {
      payload.rows.push_back(std::atoi(part.c_str()));
    }
  }
  return payload;
}

inline bool IsCrossPlaylist(int source_id, int dest_id) { return source_id >= 0 && dest_id >= 0 && source_id != dest_id; }

}  // namespace PlaylistDragPayload

#endif  // STRAWBERRY_PLAYLISTDRAGPAYLOAD_H
