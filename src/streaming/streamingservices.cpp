#include "streaming/streamingservices.h"

#include "config.h"
#ifdef HAVE_QOBUZ
#include "qobuz/qobuzservice.h"
#include "qobuz/qobuzurlhandler.h"
#endif
#ifdef HAVE_SPOTIFY
#include "spotify/spotifyservice.h"
#endif
#ifdef HAVE_SUBSONIC
#include "subsonic/subsonicservice.h"
#include "subsonic/subsonicurlhandler.h"
#endif
#ifdef HAVE_TIDAL
#include "tidal/tidalservice.h"
#include "tidal/tidalurlhandler.h"
#endif

StreamingServices::StreamingServices(NetworkAccessManager *network, UrlHandlers *url_handlers, TaskManager *task_manager) {
#ifdef HAVE_SUBSONIC
  auto subsonic = std::make_unique<SubsonicService>(network);
  auto subsonic_handler = std::make_unique<SubsonicUrlHandler>(subsonic.get());
  if (url_handlers) {
    url_handlers->AddHandler(subsonic_handler.get());
  }
  handlers_.push_back(std::move(subsonic_handler));
  services_.push_back(std::move(subsonic));
#endif
#ifdef HAVE_TIDAL
  auto tidal = std::make_unique<TidalService>(network);
  auto tidal_handler = std::make_unique<TidalUrlHandler>(tidal.get(), task_manager);
  if (url_handlers) {
    url_handlers->AddHandler(tidal_handler.get());
  }
  handlers_.push_back(std::move(tidal_handler));
  services_.push_back(std::move(tidal));
#endif
#ifdef HAVE_SPOTIFY
  auto spotify = std::make_unique<SpotifyService>(network);
  if (url_handlers) {
    url_handlers->AddHandler(spotify.get());
  }
  services_.push_back(std::move(spotify));
#endif
#ifdef HAVE_QOBUZ
  auto qobuz = std::make_unique<QobuzService>(network);
  auto qobuz_handler = std::make_unique<QobuzUrlHandler>(qobuz.get(), task_manager);
  if (url_handlers) {
    url_handlers->AddHandler(qobuz_handler.get());
  }
  handlers_.push_back(std::move(qobuz_handler));
  services_.push_back(std::move(qobuz));
#endif
  (void)network;
  (void)url_handlers;
  (void)task_manager;
}

std::vector<StreamingService *> StreamingServices::All() const {
  std::vector<StreamingService *> result;
  for (const auto &service : services_) {
    result.push_back(service.get());
  }
  return result;
}

StreamingService *StreamingServices::ServiceByName(const std::string &name) const {
  for (const auto &service : services_) {
    if (service->name() == name) {
      return service.get();
    }
  }
  return nullptr;
}
