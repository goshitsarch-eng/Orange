#ifndef STRAWBERRY_GENIUSLYRICSPROVIDER_H
#define STRAWBERRY_GENIUSLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class GeniusLyricsProvider : public HtmlLyricsProvider {
 public:
  GeniusLyricsProvider();
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;

 protected:
  std::string UrlFor(const Song &song) const override;
};

#endif
