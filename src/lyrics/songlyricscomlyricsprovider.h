#ifndef STRAWBERRY_SONGLYRICSCOMLYRICSPROVIDER_H
#define STRAWBERRY_SONGLYRICSCOMLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class SongLyricsComLyricsProvider : public HtmlLyricsProvider {
 public:
  SongLyricsComLyricsProvider();

 protected:
  std::string UrlFor(const Song &song) const override;
};

#endif
