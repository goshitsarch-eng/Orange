#include "lyrics/lyricsproviders.h"

#include "config.h"

#include "core/settings.h"
#include "lyrics/azlyricscomlyricsprovider.h"
#include "lyrics/elyricsnetlyricsprovider.h"
#include "lyrics/geniuslyricsprovider.h"
#include "lyrics/letraslyricsprovider.h"
#include "lyrics/lrcliblyricsprovider.h"
#include "lyrics/musixmatchlyricsprovider.h"
#include "lyrics/ovhlyricsprovider.h"
#include "lyrics/songlyricscomlyricsprovider.h"

LyricsProviders::LyricsProviders(NetworkAccessManager *network) : network_(network) {
  providers_.push_back(std::make_unique<LrcLibLyricsProvider>());
  providers_.push_back(std::make_unique<OVHLyricsProvider>());
  providers_.push_back(std::make_unique<GeniusLyricsProvider>());
  providers_.push_back(std::make_unique<MusixmatchLyricsProvider>());
  providers_.push_back(std::make_unique<AzLyricsComLyricsProvider>());
  providers_.push_back(std::make_unique<ElyricsNetLyricsProvider>());
  providers_.push_back(std::make_unique<LetrasLyricsProvider>());
  providers_.push_back(std::make_unique<SongLyricsComLyricsProvider>());
  ReloadSettings();
}

void LyricsProviders::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Lyrics");
  for (auto &provider : providers_) {
    const std::string legacy = [&]() {
      if (provider->name() == "azlyrics.com") return std::string("AZLyrics");
      if (provider->name() == "elyrics.net") return std::string("eLyrics");
      if (provider->name() == "letras.mus.br") return std::string("Letras");
      if (provider->name() == "songlyrics.com") return std::string("SongLyrics");
      if (provider->name() == "Lyrics.ovh") return std::string("lyrics.ovh");
      if (provider->name() == "LrcLib") return std::string("lrclib");
      return std::string();
    }();
    bool enabled = settings.BoolValue(provider->name(), true);
    if (!legacy.empty() && settings.Contains(legacy)) {
      enabled = settings.BoolValue(legacy, enabled);
    }
    provider->set_enabled(enabled);
  }
}

void LyricsProviders::Fetch(const Song &song, LyricsProvider::Callback callback) {
  if (!song.lyrics().empty()) {
    callback(song.lyrics(), {});
    return;
  }
  FetchFromIndex(song, 0, std::move(callback));
}

void LyricsProviders::FetchFromIndex(const Song &song, size_t index, LyricsProvider::Callback callback) {
  while (index < providers_.size() && !providers_[index]->enabled()) {
    ++index;
  }
  if (index >= providers_.size()) {
    callback({}, "No lyrics providers returned lyrics");
    return;
  }
  providers_[index]->Fetch(song, network_, [this, song, index, callback](const std::string &lyrics, const std::string &error) {
    if (!lyrics.empty()) {
      callback(lyrics, {});
      return;
    }
    (void)error;
    FetchFromIndex(song, index + 1, callback);
  });
}

std::vector<LyricsProvider *> LyricsProviders::All() const {
  std::vector<LyricsProvider *> result;
  for (const auto &provider : providers_) {
    result.push_back(provider.get());
  }
  return result;
}
