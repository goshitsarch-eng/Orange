#include "streaming/streamingservices.h"
#include "config.h"
#include "core/settings.h"
#include "core/logging.h"
#include "utilities/jsonutils.h"

namespace {
class GenericStreamingService : public StreamingService {
 public:
  GenericStreamingService(std::string name, std::string scheme, std::string search_url, NetworkAccessManager *network)
    : name_(std::move(name)), scheme_(std::move(scheme)), search_url_(std::move(search_url)), network_(network) {}
  std::string name() const override { return name_; }
  std::string scheme() const override { return scheme_; }
  LoadResult Load(const std::string &url, AsyncCallback callback) override {
    LoadResult result;
    result.type = LoadResult::Type::TrackAvailable;
    result.media_url = url;
    result.stream_url = url;
    if (callback) callback(result);
    return result;
  }
  void Search(const std::string &query, SearchCallback callback) override {
    if (!network_) { callback({}); return; }
    gchar *escaped = g_uri_escape_string(query.c_str(), nullptr, TRUE);
    const std::string url = search_url_ + (escaped ? escaped : query);
    g_free(escaped);
    network_->Get(url, [callback](const NetworkAccessManager::Response &response) {
      SongList songs;
      if (response.ok() && !response.body.empty()) {
        songs = JsonUtils::ParseSongs(response.body, Song::Source::Stream);
        if (songs.empty()) {
          const std::string title = JsonUtils::FindStringByKeys(response.body, {"title", "name"});
          if (!title.empty()) {
            Song song(Song::Source::Stream);
            song.set_title(title);
            song.set_valid(true);
            songs.push_back(song);
          }
        }
      }
      callback(songs);
    });
  }
  void Login(const std::string &username, const std::string &token) override {
    Settings s; s.BeginGroup(name_); s.SetValue("username", username); s.SetValue("token", token); s.Sync();
    logged_in_ = !username.empty() || !token.empty();
  }
 private:
  std::string name_, scheme_, search_url_;
  NetworkAccessManager *network_;
};
}

StreamingServices::StreamingServices(NetworkAccessManager *network, UrlHandlers *url_handlers) {
#ifdef HAVE_SUBSONIC
  auto subsonic = std::make_unique<GenericStreamingService>("Subsonic", "subsonic", "", network);
  if (url_handlers) url_handlers->AddHandler(subsonic.get());
  services_.push_back(std::move(subsonic));
#endif
#ifdef HAVE_TIDAL
  auto tidal = std::make_unique<GenericStreamingService>("Tidal", "tidal", "https://listen.tidal.com/v1/search?query=", network);
  if (url_handlers) url_handlers->AddHandler(tidal.get());
  services_.push_back(std::move(tidal));
#endif
#ifdef HAVE_SPOTIFY
  auto spotify = std::make_unique<GenericStreamingService>("Spotify", "spotify", "https://api.spotify.com/v1/search?q=", network);
  if (url_handlers) url_handlers->AddHandler(spotify.get());
  services_.push_back(std::move(spotify));
#endif
#ifdef HAVE_QOBUZ
  auto qobuz = std::make_unique<GenericStreamingService>("Qobuz", "qobuz", "https://www.qobuz.com/api.json/0.2/catalog/search?query=", network);
  if (url_handlers) url_handlers->AddHandler(qobuz.get());
  services_.push_back(std::move(qobuz));
#endif
  (void)network; (void)url_handlers;
}
std::vector<StreamingService *> StreamingServices::All() const {
  std::vector<StreamingService *> r; for (const auto &s : services_) r.push_back(s.get()); return r;
}
StreamingService *StreamingServices::ServiceByName(const std::string &name) const {
  for (const auto &s : services_) if (s->name() == name) return s.get();
  return nullptr;
}
