#include "lyrics/lyricsproviders.h"
#include "config.h"

#include <cstring>
#include "core/settings.h"
#include "utilities/strutils.h"
#include <memory>

namespace {
class HttpLyricsProvider : public LyricsProvider {
 public:
  HttpLyricsProvider(std::string name, std::string url) : name_(std::move(name)), url_(std::move(url)) {}
  std::string name() const override { return name_; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override {
    if (!network) { callback({}, "No network"); return; }
    std::string url = url_;
    auto replace = [&url](const std::string &token, const std::string &value) {
      gchar *escaped = g_uri_escape_string(value.c_str(), nullptr, TRUE);
      size_t pos = 0;
      while ((pos = url.find(token, pos)) != std::string::npos) {
        url.replace(pos, token.size(), escaped ? escaped : value);
        pos += escaped ? strlen(escaped) : value.size();
      }
      g_free(escaped);
    };
    replace("{artist}", song.artist());
    replace("{title}", song.title());
    replace("{album}", song.album());
    network->Get(url, [callback](const NetworkAccessManager::Response &response) {
      if (!response.ok()) { callback({}, response.error.empty() ? "Lyrics request failed" : response.error); return; }
      callback(response.body, {});
    });
  }
 private:
  std::string name_, url_;
};
}

LyricsProviders::LyricsProviders(NetworkAccessManager *network) : network_(network) {
  providers_.push_back(std::make_unique<HttpLyricsProvider>("Genius", "https://genius.com/api/search/song?q={artist}%20{title}"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("lyrics.ovh", "https://api.lyrics.ovh/v1/{artist}/{title}"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("Musixmatch", "https://apic-desktop.musixmatch.com/ws/1.1/macro.subtitles.get?q_artist={artist}&q_track={title}"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("SongLyrics", "https://www.songlyrics.com/{artist}/{title}-lyrics/"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("AZLyrics", "https://www.azlyrics.com/lyrics/{artist}/{title}.html"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("eLyrics", "https://www.elyrics.net/read/{artist}/{title}-lyrics.html"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("Letras", "https://www.letras.mus.br/{artist}/{title}/"));
  providers_.push_back(std::make_unique<HttpLyricsProvider>("lrclib", "https://lrclib.net/api/get?artist_name={artist}&track_name={title}&album_name={album}"));
  ReloadSettings();
}
void LyricsProviders::ReloadSettings() {
  Settings settings; settings.BeginGroup("Lyrics");
  for (auto &p : providers_) p->set_enabled(settings.BoolValue(p->name(), true));
}
void LyricsProviders::Fetch(const Song &song, LyricsProvider::Callback callback) {
  if (!song.lyrics().empty()) { callback(song.lyrics(), {}); return; }
  for (auto &p : providers_) {
    if (p->enabled()) { p->Fetch(song, network_, callback); return; }
  }
  callback({}, "No lyrics providers enabled");
}
std::vector<LyricsProvider *> LyricsProviders::All() const {
  std::vector<LyricsProvider *> r; for (const auto &p : providers_) r.push_back(p.get()); return r;
}
