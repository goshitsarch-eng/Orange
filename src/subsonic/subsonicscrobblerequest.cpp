#include "subsonic/subsonicscrobblerequest.h"

#include "utilities/strutils.h"

void SubsonicScrobbleRequest::Send(NetworkAccessManager *network, const std::string &url, const Song &song) {
  if (!network || url.empty() || song.title().empty()) {
    return;
  }
  std::string request = url;
  request += (url.find('?') == std::string::npos ? "?" : "&");
  request += "id=" + StrUtils::UriEscape(song.url()) + "&submission=true";
  network->Get(request, [](const NetworkAccessManager::Response &) {});
}
