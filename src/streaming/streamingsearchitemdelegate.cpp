#include "streaming/streamingsearchitemdelegate.h"

namespace StreamingSearchItemDelegate {

std::string PrimaryText(const Song &song) {
  if (!song.title().empty()) {
    return song.title();
  }
  if (!song.album().empty()) {
    return song.album();
  }
  if (!song.artist().empty()) {
    return song.artist();
  }
  return song.url();
}

std::string SecondaryText(const Song &song) {
  std::string text = song.artist();
  if (!song.album().empty()) {
    if (!text.empty()) {
      text += " · ";
    }
    text += song.album();
  }
  return text;
}

}  // namespace StreamingSearchItemDelegate
