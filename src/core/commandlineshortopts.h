#ifndef STRAWBERRY_COMMANDLINESHORTOPTS_H
#define STRAWBERRY_COMMANDLINESHORTOPTS_H

#include <cstring>

namespace CommandlineShortOpts {

inline constexpr char kPlay = 'p';
inline constexpr char kPlayPause = 't';
inline constexpr char kPause = 'u';
inline constexpr char kStop = 's';
inline constexpr char kPrevious = 'r';
inline constexpr char kNext = 'f';
inline constexpr char kPlayTrack = 'k';
inline constexpr char kPlayPlaylist = 'i';
inline constexpr char kShowOsd = 'o';
inline constexpr char kTogglePrettyOsd = 'y';
inline constexpr char kLanguage = 'g';
inline constexpr char kResizeWindow = 'w';
inline constexpr char kAppend = 'a';
inline constexpr char kLoad = 'l';
inline constexpr char kCreate = 'c';

// GTK keeps these letters; do not give them to Qt's --volume / --stop-after-current.
inline constexpr char kVersion = 'v';
inline constexpr char kQuiet = 'q';

inline bool IsReserved(char c) { return c == kVersion || c == kQuiet; }

inline bool IsQtCompatible(char c) {
  switch (c) {
    case kPlay:
    case kPlayPause:
    case kPause:
    case kStop:
    case kPrevious:
    case kNext:
    case kPlayTrack:
    case kPlayPlaylist:
    case kShowOsd:
    case kTogglePrettyOsd:
    case kLanguage:
    case kResizeWindow:
    case kAppend:
    case kLoad:
    case kCreate:
      return true;
    default:
      return false;
  }
}

inline char ForLongOption(const char *name) {
  if (!name) {
    return 0;
  }
  if (std::strcmp(name, "play") == 0) {
    return kPlay;
  }
  if (std::strcmp(name, "play-pause") == 0) {
    return kPlayPause;
  }
  if (std::strcmp(name, "pause") == 0) {
    return kPause;
  }
  if (std::strcmp(name, "stop") == 0) {
    return kStop;
  }
  if (std::strcmp(name, "previous") == 0) {
    return kPrevious;
  }
  if (std::strcmp(name, "next") == 0) {
    return kNext;
  }
  if (std::strcmp(name, "play-track") == 0) {
    return kPlayTrack;
  }
  if (std::strcmp(name, "play-playlist") == 0) {
    return kPlayPlaylist;
  }
  if (std::strcmp(name, "show-osd") == 0) {
    return kShowOsd;
  }
  if (std::strcmp(name, "toggle-pretty-osd") == 0) {
    return kTogglePrettyOsd;
  }
  if (std::strcmp(name, "language") == 0) {
    return kLanguage;
  }
  if (std::strcmp(name, "resize-window") == 0) {
    return kResizeWindow;
  }
  if (std::strcmp(name, "append") == 0) {
    return kAppend;
  }
  if (std::strcmp(name, "load") == 0) {
    return kLoad;
  }
  if (std::strcmp(name, "create") == 0) {
    return kCreate;
  }
  return 0;
}

}  // namespace CommandlineShortOpts

#endif
