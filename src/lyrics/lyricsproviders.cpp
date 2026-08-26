#include "lyrics/lyricsproviders.h"

#include "config.h"

#include "constants/lyricssettings.h"
#include "core/settings.h"
#include "lyrics/lyricsproviderorder.h"
#include "lyrics/lyricsprovidersettings.h"
#include "lyrics/azlyricscomlyricsprovider.h"
#include "lyrics/elyricsnetlyricsprovider.h"
#include "lyrics/geniuslyricsprovider.h"
#include "lyrics/letraslyricsprovider.h"
#include "lyrics/lrcliblyricsprovider.h"
#include "lyrics/musixmatchlyricsprovider.h"
#include "lyrics/ovhlyricsprovider.h"
#include "lyrics/songlyricscomlyricsprovider.h"

#include <algorithm>

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
  settings.BeginGroup(LyricsSettings::kSettingsGroup);
  const std::vector<std::string> order = LyricsProviderOrder::Parse(settings.Value(LyricsSettings::kProviders, ""));
  const bool has_providers = settings.Contains(LyricsSettings::kProviders);
  for (size_t i = 0; i < providers_.size(); ++i) {
    auto &provider = providers_[i];
    const std::string legacy = [&]() {
      if (provider->name() == "azlyrics.com") return std::string("AZLyrics");
      if (provider->name() == "elyrics.net") return std::string("eLyrics");
      if (provider->name() == "letras.mus.br") return std::string("Letras");
      if (provider->name() == "songlyrics.com") return std::string("SongLyrics");
      if (provider->name() == "Lyrics.ovh") return std::string("lyrics.ovh");
      if (provider->name() == "LrcLib") return std::string("lrclib");
      return std::string();
    }();
    const bool has_name_key = settings.Contains(provider->name()) || (!legacy.empty() && settings.Contains(legacy));
    bool enabled = settings.BoolValue(provider->name(), true);
    if (!legacy.empty() && settings.Contains(legacy)) {
      enabled = settings.BoolValue(legacy, enabled);
    }
    enabled = LyricsProviderSettings::EnabledFromStored(has_name_key, enabled, has_providers,
                                                        LyricsProviderSettings::InList(order, provider->name()) ||
                                                            LyricsProviderSettings::InList(order, legacy));
    provider->set_enabled(enabled);
    provider->set_order(LyricsProviderOrder::Rank(order, provider->name(), static_cast<int>(1000 + i)));
  }
  std::sort(providers_.begin(), providers_.end(), [](const std::unique_ptr<LyricsProvider> &a, const std::unique_ptr<LyricsProvider> &b) {
    return a->order() < b->order();
  });
}

void LyricsProviders::SaveOrder() {
  const std::vector<std::string> names = LyricsProviderSettings::EnabledNames(All());
  Settings settings;
  settings.BeginGroup(LyricsSettings::kSettingsGroup);
  settings.SetValue(LyricsSettings::kProviders, LyricsProviderOrder::Join(names));
  settings.Sync();
}

void LyricsProviders::SetEnabled(LyricsProvider *provider, bool enabled) {
  if (!provider) {
    return;
  }
  provider->set_enabled(enabled);
  Settings settings;
  settings.BeginGroup(LyricsSettings::kSettingsGroup);
  settings.SetBoolValue(provider->name(), enabled);
  settings.Sync();
  SaveOrder();
}

void LyricsProviders::Move(int index, int delta) {
  const int dest = index + delta;
  if (index < 0 || dest < 0 || dest >= static_cast<int>(providers_.size())) {
    return;
  }
  std::swap(providers_[static_cast<size_t>(index)], providers_[static_cast<size_t>(dest)]);
  for (size_t i = 0; i < providers_.size(); ++i) {
    providers_[i]->set_order(static_cast<int>(i));
  }
  SaveOrder();
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

LyricsProvider *LyricsProviders::ProviderByName(const std::string &name) const {
  for (const auto &provider : providers_) {
    if (provider->name() == name) {
      return provider.get();
    }
  }
  return nullptr;
}

std::vector<LyricsProvider *> LyricsProviders::All() const {
  std::vector<LyricsProvider *> result;
  for (const auto &provider : providers_) {
    result.push_back(provider.get());
  }
  return result;
}
