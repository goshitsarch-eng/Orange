#ifndef STRAWBERRY_SUBSONICURLHANDLER_H
#define STRAWBERRY_SUBSONICURLHANDLER_H

#include "core/urlhandlers.h"

#include <map>
#include <string>

class SubsonicService;

class SubsonicUrlHandler : public UrlHandler {
 public:
  explicit SubsonicUrlHandler(SubsonicService *service);

  std::string scheme() const override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;

  static std::string SongId(const std::string &url);
  static std::string StreamUrl(const std::string &server_url, const std::string &username, const std::string &password,
                               const std::string &song_id, bool hex_auth);

 private:
  SubsonicService *service_ = nullptr;
};

#endif
