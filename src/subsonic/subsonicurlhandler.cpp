#include "subsonic/subsonicurlhandler.h"

#include "streaming/streamingmediaid.h"
#include "subsonic/subsonicservice.h"

SubsonicUrlHandler::SubsonicUrlHandler(SubsonicService *service) : service_(service) {}

std::string SubsonicUrlHandler::scheme() const { return service_ ? service_->scheme() : "subsonic"; }

std::string SubsonicUrlHandler::SongId(const std::string &url) { return StreamingMediaId(url); }

std::string SubsonicUrlHandler::StreamUrl(const std::string &server_url, const std::string &username, const std::string &password,
                                          const std::string &song_id, bool hex_auth) {
  return SubsonicService::CreateUrl(server_url, username, password, "stream", {{"id", song_id}}, hex_auth);
}

UrlHandler::LoadResult SubsonicUrlHandler::Load(const std::string &url, AsyncCallback callback) {
  if (service_) {
    return service_->Load(url, std::move(callback));
  }
  LoadResult result;
  result.media_url = url;
  result.error = "Subsonic is not available";
  if (callback) {
    callback(result);
  }
  return result;
}
