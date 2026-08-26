#ifndef STRAWBERRY_JSONLYRICSPROVIDER_H
#define STRAWBERRY_JSONLYRICSPROVIDER_H

#include "lyrics/lyricsprovider.h"

#include <string>

class JsonLyricsProvider : public LyricsProvider {
 public:
  JsonLyricsProvider(std::string name, std::string url_template);
  std::string name() const override { return name_; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;

 protected:
  virtual std::string UrlFor(const Song &song) const;
  virtual std::string Extract(const std::string &body) const;

  std::string name_;
  std::string url_template_;
};

#endif
