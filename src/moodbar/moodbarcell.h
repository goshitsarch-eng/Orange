#ifndef STRAWBERRY_MOODBARCELL_H
#define STRAWBERRY_MOODBARCELL_H

#include "core/song.h"

#include <string>

namespace MoodbarCell {

enum class State {
  None = 0,
  CannotLoad = 1,
  Loading = 2,
  Loaded = 3
};

inline bool IsLocalUrl(const std::string &url) {
  if (url.empty()) {
    return false;
  }
  return url.rfind("file:", 0) == 0 || url[0] == '/';
}

inline bool CanLoad(const Song &song) {
  if (song.is_stream() || song.is_cdda() || !song.cue_path().empty()) {
    return false;
  }
  return IsLocalUrl(song.url());
}

inline std::string CacheKey(const Song &song) { return song.url(); }

inline int ColumnHeight() { return 16; }

inline int ColumnWidth() { return 120; }

inline int BorderInset() { return 1; }

inline const char *PlaceholderText() { return ""; }

inline State NextState(State current, bool can_load, bool has_data, bool loading) {
  if (!can_load) {
    return State::CannotLoad;
  }
  if (has_data) {
    return State::Loaded;
  }
  if (loading || current == State::Loading) {
    return State::Loading;
  }
  if (current == State::CannotLoad) {
    return State::CannotLoad;
  }
  return State::None;
}

}  // namespace MoodbarCell

#endif
