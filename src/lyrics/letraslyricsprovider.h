#ifndef STRAWBERRY_LETRASLYRICSPROVIDER_H
#define STRAWBERRY_LETRASLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class LetrasLyricsProvider : public HtmlLyricsProvider {
 public:
  LetrasLyricsProvider();

 protected:
  std::string UrlFor(const Song &song) const override;
  std::map<std::string, std::string> RequestHeaders() const override;
};

#endif
