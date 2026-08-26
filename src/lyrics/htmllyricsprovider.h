#ifndef STRAWBERRY_HTMLLYRICSPROVIDER_H
#define STRAWBERRY_HTMLLYRICSPROVIDER_H

#include "lyrics/lyricsproviders.h"

#include <map>
#include <string>

class HtmlLyricsProvider : public LyricsProvider {
 public:
  HtmlLyricsProvider(std::string name, std::string start_tag, std::string end_tag, std::string lyrics_start, bool multiple);

  std::string name() const override { return name_; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;

  static std::string ParseLyricsFromHTML(const std::string &content, const std::string &start_tag, const std::string &end_tag,
                                         const std::string &lyrics_start, bool multiple);
  static std::string SlugAzLyrics(const std::string &text);
  static std::string SlugDashed(const std::string &text);
  static std::string SlugElyrics(const std::string &text);
  static std::string SlugLetras(const std::string &text);

 protected:
  virtual std::string UrlFor(const Song &song) const = 0;
  virtual std::map<std::string, std::string> RequestHeaders() const { return {}; }

  std::string name_;
  std::string start_tag_;
  std::string end_tag_;
  std::string lyrics_start_;
  bool multiple_ = false;
};

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
