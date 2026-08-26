#ifndef STRAWBERRY_PLAYLISTLISTDROP_H
#define STRAWBERRY_PLAYLISTLISTDROP_H

#include "queue/queuedrop.h"

#include <string>
#include <vector>

namespace PlaylistListDrop {

struct Row {
  std::string name;
  bool favorite = false;
};

inline std::string DisplayName(const std::string &name, bool favorite) { return (favorite ? "★ " : "") + name; }

inline bool IsPlaylistRows(const std::string &text) { return QueueDrop::IsPlaylistRows(text); }

inline std::vector<int> ParsePlaylistRows(const std::string &text) { return QueueDrop::ParseRows(text, QueueDrop::kPlaylistRowsPrefix); }

inline std::vector<std::string> ParseUrls(const std::string &text) { return QueueDrop::ParseUrls(text); }

}  // namespace PlaylistListDrop

#endif
