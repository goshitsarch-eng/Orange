#ifndef STRAWBERRY_TIDALSERVICE_H
#define STRAWBERRY_TIDALSERVICE_H

#include "streaming/streamingservices.h"

#include <map>
#include <string>

class TidalService : public StreamingService {
 public:
  static const char kApiUrl[];

  explicit TidalService(NetworkAccessManager *network);

  std::string name() const override { return "Tidal"; }
  std::string scheme() const override { return "tidal"; }
  void Search(const std::string &query, SearchCallback callback) override;
  void Login(const std::string &username, const std::string &password_or_token) override;
  void ReloadSettings() override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;

 private:
  std::map<std::string, std::string> AuthHeaders() const;
  std::string TrackId(const std::string &url) const;

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
  std::string country_code_ = "US";
};

#endif
