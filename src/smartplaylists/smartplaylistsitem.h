#ifndef STRAWBERRY_SMARTPLAYLISTSITEM_H
#define STRAWBERRY_SMARTPLAYLISTSITEM_H

#include "smartplaylists/smartplaylist.h"

#include <string>

struct SmartPlaylistsItem {
  enum class Kind { Builtin, Saved, Wizard };

  Kind kind = Kind::Builtin;
  std::string title;
  std::string key;
  SmartPlaylistSearch search;
};

#endif
