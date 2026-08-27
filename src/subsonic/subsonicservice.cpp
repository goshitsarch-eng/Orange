#include "subsonic/subsonicservice.h"

#include "constants/subsonicsettings.h"
#include "core/settings.h"
#include "subsonic/subsonicfavoriterequest.h"
#include "subsonic/subsonicrequest.h"
#include "streaming/streamingalbum.h"
#include "streaming/streamingcoverdownload.h"
#include "streaming/streamingsearchopts.h"
#include "subsonic/subsonicurlhandler.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>

const char *SubsonicService::kClientName = "Strawberry";
const char *SubsonicService::kApiVersion = "1.11.0";

namespace {

std::string JoinPath(const std::string &server, const std::string &resource) {
  std::string url = server;
  if (!url.empty() && url.back() == '/') {
    url.pop_back();
  }
  return url + "/rest/" + resource + ".view";
}

}  // namespace

SubsonicService::SubsonicService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

std::string SubsonicService::Md5Hex(const std::string &value) {
  gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, value.c_str(), static_cast<gssize>(value.size()));
  std::string result = digest ? digest : "";
  g_free(digest);
  return result;
}

std::string SubsonicService::HexEncode(const std::string &value) {
  std::string hex;
  hex.reserve(value.size() * 2);
  for (unsigned char ch : value) {
    char buf[3];
    g_snprintf(buf, sizeof(buf), "%02x", ch);
    hex += buf;
  }
  return hex;
}

std::string SubsonicService::RandomSalt(int length) {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  std::string salt;
  salt.reserve(static_cast<size_t>(length));
  for (int i = 0; i < length; ++i) {
    salt.push_back(alphabet[g_random_int_range(0, static_cast<gint32>(sizeof(alphabet) - 1))]);
  }
  return salt;
}

std::string SubsonicService::CreateUrl(const std::string &server_url, const std::string &username, const std::string &password,
                                       const std::string &resource, const std::map<std::string, std::string> &params, bool hex_auth) {
  std::string url = JoinPath(server_url, resource);
  url += "?c=" + StrUtils::UriEscape(kClientName) + "&v=" + kApiVersion + "&f=json&u=" + StrUtils::UriEscape(username);
  if (hex_auth) {
    url += "&p=enc:" + HexEncode(password);
  } else {
    const std::string salt = RandomSalt();
    url += "&s=" + salt + "&t=" + Md5Hex(password + salt);
  }
  for (const auto &param : params) {
    url += "&" + StrUtils::UriEscape(param.first) + "=" + StrUtils::UriEscape(param.second);
  }
  return url;
}

SongList SubsonicService::WithCoverUrls(SongList songs) const {
  StreamingCoverDownload::ApplyCoverArtIds(songs, [this](const std::string &id) {
    return CreateUrl(server_url_, username_, password_, "getCoverArt", {{"id", id}}, hex_auth_);
  });
  return songs;
}

void SubsonicService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  server_url_ = settings.Value(SubsonicSettings::kUrl);
  username_ = settings.Value(SubsonicSettings::kUsername);
  password_ = settings.Value(SubsonicSettings::kPassword);
  if (password_.empty()) {
    password_ = settings.Value("token");
  }
  hex_auth_ = settings.IntValue(SubsonicSettings::kAuthMethod, static_cast<int>(SubsonicSettings::kDefaultAuthMethod)) ==
                  static_cast<int>(SubsonicSettings::AuthMethod::Hex) ||
              settings.Value("auth") == "hex";
  logged_in_ = !server_url_.empty() && !username_.empty() && !password_.empty();
}

void SubsonicService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup("Subsonic");
  settings.SetValue("username", username);
  settings.SetValue("password", password_or_token);
  settings.SetValue("token", password_or_token);
  settings.Sync();
  ReloadSettings();
}

void SubsonicService::Search(const std::string &query, SearchCallback callback) { Search(query, SearchType::Songs, std::move(callback)); }

void SubsonicService::Search(const std::string &query, SearchType type, SearchCallback callback) {
  auto guarded = GuardSearch(std::move(callback));
  if (!logged_in_) {
    guarded({});
    return;
  }
  const int gen = search_generation();
  const auto request_type = SubsonicRequest::FromSearchType(type);
  const int limit = StreamingSearchOpts::LimitFor(name(), type);
  SubsonicRequest::GetAll(
      network_,
      [this, request_type, query](int offset, int page_limit) {
        return CreateUrl(server_url_, username_, password_, SubsonicRequest::Resource(request_type),
                         SubsonicRequest::Params(request_type, query, offset, page_limit), hex_auth_);
      },
      request_type,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, {}, WithCoverUrls(songs), guarded,
                          [this](const std::string &text) { SearchUpdateStatus.Emit(last_search_id_, text); },
                          [this](int received, int total) { ReportSearchProgress(received, total); },
                          [this, gen]() { return SearchRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportSearchProgress(received, total); }, [this, gen]() { return SearchRequestCurrent(gen); }, limit, limit,
      [this](const std::string &error) { NotifySearchFailed(error); });
}

void SubsonicService::GetArtists(SearchCallback callback) {
  auto guarded = GuardArtists(std::move(callback));
  if (!logged_in_) {
    guarded({});
    return;
  }
  SubsonicRequest::Get(network_, CreateUrl(server_url_, username_, password_, SubsonicRequest::Resource(SubsonicRequest::Type::ArtistsList),
                                           {}, hex_auth_),
                       SubsonicRequest::Type::ArtistsList,
                       [this, guarded](const SongList &songs) { DeliverWithCovers(network_, {}, WithCoverUrls(songs), guarded); },
                       [this](const std::string &error) { NotifyArtistsFailed(error); });
}

void SubsonicService::GetAlbums(SearchCallback callback) {
  auto guarded = GuardAlbums(std::move(callback));
  if (!logged_in_) {
    guarded({});
    return;
  }
  const int gen = albums_generation();
  SubsonicRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return CreateUrl(server_url_, username_, password_, SubsonicRequest::Resource(SubsonicRequest::Type::AlbumList),
                         SubsonicRequest::Params(SubsonicRequest::Type::AlbumList, {}, offset, limit), hex_auth_);
      },
      SubsonicRequest::Type::AlbumList,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, {}, WithCoverUrls(songs), guarded, [this](const std::string &text) { AlbumsUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportAlbumsProgress(received, total); },
                          [this, gen]() { return AlbumsRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportAlbumsProgress(received, total); }, [this, gen]() { return AlbumsRequestCurrent(gen); },
      StreamingPage::kDefaultLimit, 0,
      [this](const std::string &error) { NotifyAlbumsFailed(error); });
}

void SubsonicService::GetSongs(SearchCallback callback) {
  auto guarded = GuardSongs(std::move(callback));
  if (!logged_in_) {
    guarded({});
    return;
  }
  const int gen = songs_generation();
  SubsonicRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return CreateUrl(server_url_, username_, password_, SubsonicRequest::Resource(SubsonicRequest::Type::SearchSongs),
                         SubsonicRequest::Params(SubsonicRequest::Type::SearchSongs, ".", offset, limit), hex_auth_);
      },
      SubsonicRequest::Type::SearchSongs,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, {}, WithCoverUrls(songs), guarded, [this](const std::string &text) { SongsUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportSongsProgress(received, total); },
                          [this, gen]() { return SongsRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportSongsProgress(received, total); }, [this, gen]() { return SongsRequestCurrent(gen); },
      StreamingPage::kDefaultLimit, 0,
      [this](const std::string &error) { NotifySongsFailed(error); });
}

void SubsonicService::GetArtistAlbums(const Song &artist, SearchCallback callback) {
  if (!logged_in_ || artist.artist_id().empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  SubsonicRequest::Get(network_,
                       CreateUrl(server_url_, username_, password_, SubsonicRequest::Resource(SubsonicRequest::Type::ArtistAlbums),
                                 SubsonicRequest::ArtistAlbumsParams(artist.artist_id()), hex_auth_),
                       SubsonicRequest::Type::ArtistAlbums,
                       [this, callback](const SongList &songs) { DeliverWithCovers(network_, {}, WithCoverUrls(songs), callback); });
}

void SubsonicService::GetAlbumSongs(const Song &album, SearchCallback callback) {
  if (!logged_in_ || album.album_id().empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  SubsonicRequest::Get(network_,
                       CreateUrl(server_url_, username_, password_, SubsonicRequest::Resource(SubsonicRequest::Type::AlbumSongs),
                                 SubsonicRequest::AlbumSongsParams(album.album_id()), hex_auth_),
                       SubsonicRequest::Type::AlbumSongs, [this, album, callback](const SongList &songs) {
                         SongList copy = songs;
                         StreamingAlbum::ApplyParent(copy, album);
                         DeliverWithCovers(network_, {}, WithCoverUrls(copy), callback);
                       });
}

UrlHandler::LoadResult SubsonicService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  const std::string id = SubsonicUrlHandler::SongId(url);
  if (!logged_in_ || id.empty()) {
    result.error = "Subsonic is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::TrackAvailable;
  result.stream_url = SubsonicUrlHandler::StreamUrl(server_url_, username_, password_, id, hex_auth_);
  if (callback) {
    callback(result);
  }
  return result;
}

void SubsonicService::GetFavorites(FavoriteType, SearchCallback callback) {
  auto guarded = GuardFavorites(std::move(callback));
  const int gen = favorites_generation();
  if (!logged_in_) {
    guarded({});
    return;
  }
  SubsonicFavoriteRequest::Get(
      network_, CreateUrl(server_url_, username_, password_, "getStarred2", {}, hex_auth_),
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, {}, WithCoverUrls(songs), guarded, [this](const std::string &text) { FavoritesUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportFavoritesProgress(received, total); },
                          [this, gen]() { return FavoritesRequestCurrent(gen); });
      },
      [this](const std::string &error) { NotifyFavoritesFailed(error); });
}

void SubsonicService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  const auto ids = SubsonicFavoriteRequest::IdsFromSongs(type, songs);
  if (!logged_in_ || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  std::vector<std::string> urls;
  for (const auto &params : SubsonicFavoriteRequest::StarParamsForIds(type, ids)) {
    urls.push_back(CreateUrl(server_url_, username_, password_, SubsonicFavoriteRequest::StarResource(false), params, hex_auth_));
  }
  SubsonicFavoriteRequest::MutateMany(network_, urls, songs, std::move(callback));
}

void SubsonicService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  const auto ids = SubsonicFavoriteRequest::IdsFromSongs(type, songs);
  if (!logged_in_ || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  std::vector<std::string> urls;
  for (const auto &params : SubsonicFavoriteRequest::StarParamsForIds(type, ids)) {
    urls.push_back(CreateUrl(server_url_, username_, password_, SubsonicFavoriteRequest::StarResource(true), params, hex_auth_));
  }
  SubsonicFavoriteRequest::MutateMany(network_, urls, songs, std::move(callback));
}
