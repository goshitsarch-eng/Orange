#ifndef STRAWBERRY_PLAYLISTLISTDROP_H
#define STRAWBERRY_PLAYLISTLISTDROP_H

#include "playlist/playlistdragpayload.h"
#include "queue/queuedrop.h"

#include <string>
#include <vector>

namespace PlaylistListDrop {

struct Row {
  std::string name;
  bool favorite = false;
  bool folder = false;
  std::string path;
  int depth = 0;
  bool expanded = true;
  int id = -1;
};

inline constexpr const char *kMovePrefix = "strawberry-playlist-move:";

inline std::string DisplayName(const std::string &name, bool favorite) { return (favorite ? "★ " : "") + name; }

inline bool IsPlaylistMove(const std::string &text) { return text.rfind(kMovePrefix, 0) == 0; }

inline std::string MovePayload(const std::string &name) { return std::string(kMovePrefix) + name; }

inline std::string ParseMoveName(const std::string &text) {
  if (!IsPlaylistMove(text)) {
    return {};
  }
  return text.substr(std::string(kMovePrefix).size());
}

inline bool IsPlaylistRows(const std::string &text) { return PlaylistDragPayload::IsPlaylistRows(text); }

inline std::vector<int> ParsePlaylistRows(const std::string &text) { return PlaylistDragPayload::Decode(text).rows; }

inline std::vector<std::string> ParseUrls(const std::string &text) { return QueueDrop::ParseUrls(text); }

}  // namespace PlaylistListDrop

#endif
