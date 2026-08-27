#ifndef STRAWBERRY_SUBSONICSCROBBLEREQUEST_H
#define STRAWBERRY_SUBSONICSCROBBLEREQUEST_H

#include "core/network.h"
#include "core/song.h"

#include <string>

class SubsonicScrobbleRequest {
 public:
  static std::string Resource() { return "scrobble"; }
  static void Send(NetworkAccessManager *network, const std::string &url, const Song &song);
};

#endif
