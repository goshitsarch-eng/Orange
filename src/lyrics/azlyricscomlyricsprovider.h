#ifndef STRAWBERRY_AZLYRICSCOMLYRICSPROVIDER_H
#define STRAWBERRY_AZLYRICSCOMLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class AzLyricsComLyricsProvider : public HtmlLyricsProvider {
 public:
  AzLyricsComLyricsProvider();

 protected:
  std::string UrlFor(const Song &song) const override;
};

#endif
