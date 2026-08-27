#ifndef STRAWBERRY_LYRICSSEARCHREQUEST_H
#define STRAWBERRY_LYRICSSEARCHREQUEST_H

#include <cstdint>
#include <string>

class LyricsSearchRequest {
 public:
  LyricsSearchRequest() = default;
  std::string albumartist;
  std::string artist;
  std::string album;
  std::string title;
  int64_t duration = -1;
};

#endif
