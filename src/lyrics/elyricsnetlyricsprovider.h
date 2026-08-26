#ifndef STRAWBERRY_ELYRICSNETLYRICSPROVIDER_H
#define STRAWBERRY_ELYRICSNETLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class ElyricsNetLyricsProvider : public HtmlLyricsProvider {
 public:
  ElyricsNetLyricsProvider();

 protected:
  std::string UrlFor(const Song &song) const override;
};

#endif
