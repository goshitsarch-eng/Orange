#ifndef STRAWBERRY_PLAYLISTITEMUUID_H
#define STRAWBERRY_PLAYLISTITEMUUID_H

#include <glib.h>

#include <string>

namespace PlaylistItemUuid {

inline std::string New() {
  char *uuid = g_uuid_string_random();
  std::string text = uuid ? uuid : "";
  g_free(uuid);
  return text;
}

inline bool Valid(const std::string &uuid) { return !uuid.empty(); }

}  // namespace PlaylistItemUuid

#endif  // STRAWBERRY_PLAYLISTITEMUUID_H
