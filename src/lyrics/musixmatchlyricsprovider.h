#ifndef STRAWBERRY_MUSIXMATCHLYRICSPROVIDER_H
#define STRAWBERRY_MUSIXMATCHLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class MusixmatchLyricsProvider : public JsonLyricsProvider {
 public:
  MusixmatchLyricsProvider();

 protected:
  std::string UrlFor(const Song &song) const override;
  std::string Extract(const std::string &body) const override;
};

#endif
