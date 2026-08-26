#ifndef STRAWBERRY_PLAYLISTCLIPBOARD_H
#define STRAWBERRY_PLAYLISTCLIPBOARD_H

#include <string>
#include <vector>

namespace PlaylistClipboard {

inline std::string DisplayText(const std::vector<std::string> &column_texts) {
  std::string out;
  for (const std::string &part : column_texts) {
    if (part.empty()) {
      continue;
    }
    if (!out.empty()) {
      out += " - ";
    }
    out += part;
  }
  return out;
}

inline bool IsCopyShortcut(const unsigned keyval, const unsigned modifiers, const unsigned control_mask) {
  return (modifiers & control_mask) != 0 && (keyval == 'c' || keyval == 'C');
}

}  // namespace PlaylistClipboard

#endif
