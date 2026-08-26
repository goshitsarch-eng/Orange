#include "covermanager/coverproviders.h"

#include "config.h"
#include "core/logging.h"

#include <cstring>
#include "constants/coverssettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "lyrics/lyricsproviderorder.h"
#include "covermanager/coverprovidersettings.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/deezercoverprovider.h"
#include "covermanager/discogscoverprovider.h"
#include "covermanager/lastfmcoverprovider.h"
#include "covermanager/musicbrainzcoverprovider.h"
#include "covermanager/musixmatchcoverprovider.h"
#include "covermanager/opentidalcoverprovider.h"
#include "covermanager/qobuzcoverprovider.h"
#include "covermanager/spotifycoverprovider.h"
#include "covermanager/tidalcoverprovider.h"
#include "dialogs/edittagcover.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <glib/gstdio.h>
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>

CoverProviders::CoverProviders(NetworkAccessManager *network) : network_(network) {
  auto add = [this](std::unique_ptr<CoverProvider> provider, float quality) {
    provider->set_quality(quality);
    provider->set_order(static_cast<int>(providers_.size()));
    providers_.push_back(std::move(provider));
  };
  add(std::make_unique<LastFmCoverProvider>(), 1.0f);
  add(std::make_unique<MusicbrainzCoverProvider>(), 1.5f);
  add(std::make_unique<DiscogsCoverProvider>(), 0.0f);
  add(std::make_unique<DeezerCoverProvider>(), 2.0f);
  add(std::make_unique<MusixmatchCoverProvider>(), 1.0f);
  add(std::make_unique<OpenTidalCoverProvider>(), 2.5f);
#ifdef HAVE_TIDAL
  add(std::make_unique<TidalCoverProvider>(), 2.5f);
#endif
#ifdef HAVE_SPOTIFY
  add(std::make_unique<SpotifyCoverProvider>(), 2.5f);
#endif
#ifdef HAVE_QOBUZ
  add(std::make_unique<QobuzCoverProvider>(), 2.0f);
#endif
  ReloadSettings();
}

void CoverProviders::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(CoversSettings::kSettingsGroup);
  const std::vector<std::string> order = LyricsProviderOrder::Parse(settings.Value(CoversSettings::kProviders, ""));
  const bool has_providers = settings.Contains(CoversSettings::kProviders);
  for (size_t i = 0; i < providers_.size(); ++i) {
    auto &provider = providers_[i];
    const bool has_name_key = settings.Contains(provider->name());
    const bool stored = settings.BoolValue(provider->name(), provider->enabled());
    provider->set_enabled(CoverProviderSettings::EnabledFromStored(has_name_key, stored, has_providers,
                                                                   CoverProviderSettings::InList(order, provider->name()), provider->enabled()));
    provider->set_order(LyricsProviderOrder::Rank(order, provider->name(), static_cast<int>(1000 + i)));
  }
  std::sort(providers_.begin(), providers_.end(), [](const std::unique_ptr<CoverProvider> &a, const std::unique_ptr<CoverProvider> &b) {
    return a->order() < b->order();
  });
}

void CoverProviders::SaveOrder() {
  const std::vector<std::string> names = CoverProviderSettings::EnabledNames(All());
  Settings settings;
  settings.BeginGroup(CoversSettings::kSettingsGroup);
  settings.SetValue(CoversSettings::kProviders, LyricsProviderOrder::Join(names));
  settings.Sync();
}

void CoverProviders::SetEnabled(CoverProvider *provider, bool enabled) {
  if (!provider) {
    return;
  }
  provider->set_enabled(enabled);
  Settings settings;
  settings.BeginGroup(CoversSettings::kSettingsGroup);
  settings.SetBoolValue(provider->name(), enabled);
  settings.Sync();
  SaveOrder();
}

void CoverProviders::Move(int index, int delta) {
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

void CoverProviders::Fetch(const Song &song, CoverProvider::Callback callback) {
  FetchFromIndex(song, 0, std::move(callback));
}

void CoverProviders::FetchAll(const Song &song, const std::function<void(const std::string &provider, const std::string &image_data)> &callback) {
  const CoverSearchRequest request = AlbumCoverFetcherSearch::RequestFromSong(song);
  for (auto &provider : providers_) {
    if (!AlbumCoverFetcherSearch::ShouldUseProvider(provider.get(), request)) {
      continue;
    }
    const std::string name = provider->name();
    provider->Fetch(song, network_, [this, callback, name](const std::string &data, const std::string &) {
      if (JsonUtils::LooksLikeImage(data)) {
        callback(name, data);
        return;
      }
      if (!data.empty() && (StrUtils::StartsWith(data, "http://") || StrUtils::StartsWith(data, "https://"))) {
        network_->Get(data, [callback, name](const NetworkAccessManager::Response &response) {
          if (response.ok() && JsonUtils::LooksLikeImage(response.body)) {
            callback(name, response.body);
          }
        });
      }
    });
  }
}

void CoverProviders::FetchFromIndex(const Song &song, size_t index, CoverProvider::Callback callback) {
  const CoverSearchRequest request = AlbumCoverFetcherSearch::RequestFromSong(song);
  while (index < providers_.size() && !AlbumCoverFetcherSearch::ShouldUseProvider(providers_[index].get(), request)) {
    ++index;
  }
  if (index >= providers_.size()) {
    callback({}, "No cover providers returned artwork");
    return;
  }
  providers_[index]->Fetch(song, network_, [this, song, index, callback](const std::string &data, const std::string &error) {
    if (JsonUtils::LooksLikeImage(data)) {
      callback(data, {});
      return;
    }
    if (!data.empty() && (StrUtils::StartsWith(data, "http://") || StrUtils::StartsWith(data, "https://"))) {
      network_->Get(data, [this, song, index, callback](const NetworkAccessManager::Response &response) {
        if (response.ok() && JsonUtils::LooksLikeImage(response.body)) {
          callback(response.body, {});
          return;
        }
        FetchFromIndex(song, index + 1, callback);
      });
      return;
    }
    (void)error;
    FetchFromIndex(song, index + 1, callback);
  });
}

std::vector<CoverProvider *> CoverProviders::All() const {
  std::vector<CoverProvider *> result;
  for (const auto &provider : providers_) {
    result.push_back(provider.get());
  }
  return result;
}

bool CoverProviders::SaveAlbumCover(const Song &song, const std::string &image_data, TagReader *tagreader, std::string *saved_path) {
  return SaveAlbumCover(song, image_data, tagreader, CoverOptions::LoadFromSettings(), saved_path);
}

bool CoverProviders::SaveAlbumCover(const Song &song, const std::string &image_data, TagReader *tagreader, const CoverOptions &options,
                                    std::string *saved_path) {
  const std::string bytes = EditTagCover::ImageBytes(image_data);
  if (bytes.empty()) {
    return false;
  }
  const CoverOptions::CoverType type = options.EffectiveType(song);
  if (type == CoverOptions::CoverType::Embedded) {
    if (!tagreader) {
      return false;
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    if (path.empty() || !FileUtils::Exists(path)) {
      return false;
    }
    TagReader::CoverData cover;
    cover.data.assign(bytes.begin(), bytes.end());
    cover.mime_type = "image/jpeg";
    if (!tagreader->SaveCover(path, cover)) {
      return false;
    }
    if (saved_path) {
      *saved_path = path;
    }
    return true;
  }
  const std::string dest = CoverOptions::UniquePath(options.FilePath(song), options.cover_overwrite);
  g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
  if (!FileUtils::WriteFile(dest, bytes)) {
    return false;
  }
  if (saved_path) {
    *saved_path = dest;
  }
  return true;
}

void CoverProviders::FetchFromEmbeddedOrFile(const Song &song, CoverProvider::Callback callback) {
  if (!song.art_manual().empty() && FileUtils::Exists(FileUtils::PathFromUri(song.art_manual()))) {
    callback(FileUtils::ReadFile(FileUtils::PathFromUri(song.art_manual())), {});
    return;
  }
  Fetch(song, callback);
}
