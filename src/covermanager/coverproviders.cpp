#include "covermanager/coverproviders.h"

#include "config.h"
#include "core/logging.h"

#include <cstring>
#include "core/settings.h"
#include "core/standardpaths.h"
#include "covermanager/deezercoverprovider.h"
#include "covermanager/discogscoverprovider.h"
#include "covermanager/lastfmcoverprovider.h"
#include "covermanager/musicbrainzcoverprovider.h"
#include "covermanager/musixmatchcoverprovider.h"
#include "covermanager/qobuzcoverprovider.h"
#include "covermanager/spotifycoverprovider.h"
#include "covermanager/tidalcoverprovider.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <glib/gstdio.h>
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>

namespace {

class JsonCoverProvider : public CoverProvider {
 public:
  JsonCoverProvider(std::string name, std::string url_template) : name_(std::move(name)), url_template_(std::move(url_template)) {}
  std::string name() const override { return name_; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override {
    if (!network || song.album().empty()) {
      callback({}, "No album");
      return;
    }
    std::string url = url_template_;
    auto replace = [&url](const std::string &token, const std::string &value) {
      gchar *escaped = g_uri_escape_string(value.c_str(), nullptr, TRUE);
      size_t pos = 0;
      while ((pos = url.find(token, pos)) != std::string::npos) {
        url.replace(pos, token.size(), escaped ? escaped : value);
        pos += escaped ? strlen(escaped) : value.size();
      }
      g_free(escaped);
    };
    replace("{artist}", song.EffectiveAlbumartist());
    replace("{album}", song.album());
    replace("{title}", song.title());
    network->Get(url, [callback](const NetworkAccessManager::Response &response) {
      if (!response.ok()) {
        callback({}, response.error.empty() ? "Cover request failed" : response.error);
        return;
      }
      if (JsonUtils::LooksLikeImage(response.body)) {
        callback(response.body, {});
        return;
      }
      const std::string image_url = JsonUtils::FindCoverUrl(response.body);
      if (image_url.empty()) {
        callback({}, "No cover URL in provider response");
        return;
      }
      callback(image_url, {});
    });
  }
 private:
  std::string name_;
  std::string url_template_;
};

class OpenTidalCoverProvider : public JsonCoverProvider {
 public:
  OpenTidalCoverProvider() : JsonCoverProvider("OpenTidal", "https://openapi.tidal.com/v2/searchResults/%7Bquery%7D/relationships/albums") {}
};

}  // namespace

CoverProviders::CoverProviders(NetworkAccessManager *network) : network_(network) {
  providers_.push_back(std::make_unique<LastFmCoverProvider>());
  providers_.push_back(std::make_unique<MusicbrainzCoverProvider>());
  providers_.push_back(std::make_unique<DiscogsCoverProvider>());
  providers_.push_back(std::make_unique<DeezerCoverProvider>());
  providers_.push_back(std::make_unique<MusixmatchCoverProvider>());
  providers_.push_back(std::make_unique<OpenTidalCoverProvider>());
#ifdef HAVE_TIDAL
  providers_.push_back(std::make_unique<TidalCoverProvider>());
#endif
#ifdef HAVE_SPOTIFY
  providers_.push_back(std::make_unique<SpotifyCoverProvider>());
#endif
#ifdef HAVE_QOBUZ
  providers_.push_back(std::make_unique<QobuzCoverProvider>());
#endif
  ReloadSettings();
}

void CoverProviders::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Covers");
  for (auto &provider : providers_) {
    provider->set_enabled(settings.BoolValue(provider->name(), true));
  }
}

void CoverProviders::Fetch(const Song &song, CoverProvider::Callback callback) {
  FetchFromIndex(song, 0, std::move(callback));
}

void CoverProviders::FetchAll(const Song &song, const std::function<void(const std::string &provider, const std::string &image_data)> &callback) {
  for (auto &provider : providers_) {
    if (!provider->enabled()) {
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
  while (index < providers_.size() && !providers_[index]->enabled()) {
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

bool CoverProviders::SaveAlbumCover(const Song &song, const std::string &image_data, TagReader *tagreader) {
  if (image_data.empty()) {
    return false;
  }
  const std::string path = FileUtils::PathFromUri(song.url());
  const std::string dir = FileUtils::DirName(path);
  Settings settings;
  settings.BeginGroup("Covers");
  const std::string dest_mode = settings.Value("save_dest", "album");
  const std::string filename = settings.Value("filename", "cover.jpg");
  if (dest_mode == "cache") {
    const std::string cache = FileUtils::Join(g_get_user_cache_dir(), "strawberry/covers");
    g_mkdir_with_parents(cache.c_str(), 0755);
    const std::string dest = FileUtils::Join(cache, FileUtils::BaseName(path) + "-" + filename);
    return FileUtils::WriteFile(dest, image_data);
  }
  bool wrote = dest_mode == "embedded";
  if (dest_mode != "embedded") {
    const std::string dest = FileUtils::Join(dir.empty() ? "." : dir, filename.empty() ? "cover.jpg" : filename);
    wrote = FileUtils::WriteFile(dest, image_data);
  }
  if ((dest_mode == "embedded" || dest_mode == "album") && tagreader && !path.empty() && FileUtils::Exists(path)) {
    TagReader::CoverData cover;
    cover.data.assign(image_data.begin(), image_data.end());
    cover.mime_type = "image/jpeg";
    wrote = tagreader->SaveCover(path, cover) || wrote;
  }
  return wrote;
}

void CoverProviders::FetchFromEmbeddedOrFile(const Song &song, CoverProvider::Callback callback) {
  if (!song.art_manual().empty() && FileUtils::Exists(FileUtils::PathFromUri(song.art_manual()))) {
    callback(FileUtils::ReadFile(FileUtils::PathFromUri(song.art_manual())), {});
    return;
  }
  Fetch(song, callback);
}

AlbumCoverLoader::AlbumCoverLoader(TagReader *tagreader) : tagreader_(tagreader) {}

std::string AlbumCoverLoader::LoadPath(const Song &song) const {
  if (!song.art_manual().empty()) return song.art_manual();
  if (!song.art_automatic().empty()) return song.art_automatic();
  const std::string dir = FileUtils::DirName(FileUtils::PathFromUri(song.url()));
  for (const char *name : {"cover.jpg", "cover.png", "folder.jpg", "front.jpg", "album.jpg"}) {
    const std::string candidate = FileUtils::Join(dir, name);
    if (FileUtils::Exists(candidate)) {
      return FileUtils::UriFromPath(candidate);
    }
  }
  return {};
}

std::vector<unsigned char> AlbumCoverLoader::LoadData(const Song &song) const {
  if (tagreader_ && song.art_embedded()) {
    auto cover = tagreader_->LoadCoverData(FileUtils::PathFromUri(song.url()));
    if (!cover.data.empty()) return cover.data;
  }
  const std::string path = FileUtils::PathFromUri(LoadPath(song));
  if (!path.empty() && FileUtils::Exists(path)) {
    const std::string data = FileUtils::ReadFile(path);
    return std::vector<unsigned char>(data.begin(), data.end());
  }
  return {};
}

CurrentAlbumCoverLoader::CurrentAlbumCoverLoader(AlbumCoverLoader *loader) : loader_(loader) {}

void CurrentAlbumCoverLoader::Load(const Song &song) {
  current_ = loader_ ? loader_->LoadData(song) : std::vector<unsigned char>{};
  AlbumCoverReady.Emit(song, current_);
}
