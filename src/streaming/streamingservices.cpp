#include "streaming/streamingservices.h"

#include "config.h"
#ifdef HAVE_QOBUZ
#include "qobuz/qobuzservice.h"
#endif
#ifdef HAVE_SPOTIFY
#include "spotify/spotifyservice.h"
#endif
#ifdef HAVE_SUBSONIC
#include "subsonic/subsonicservice.h"
#endif
#ifdef HAVE_TIDAL
#include "tidal/tidalservice.h"
#endif

StreamingServices::StreamingServices(NetworkAccessManager *network, UrlHandlers *url_handlers) {
#ifdef HAVE_SUBSONIC
  auto subsonic = std::make_unique<SubsonicService>(network);
  if (url_handlers) {
    url_handlers->AddHandler(subsonic.get());
  }
  services_.push_back(std::move(subsonic));
#endif
#ifdef HAVE_TIDAL
  auto tidal = std::make_unique<TidalService>(network);
  if (url_handlers) {
    url_handlers->AddHandler(tidal.get());
  }
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
  if (url_handlers) {
    url_handlers->AddHandler(qobuz.get());
  }
  services_.push_back(std::move(qobuz));
#endif
  (void)network;
  (void)url_handlers;
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
