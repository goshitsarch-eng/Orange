#ifndef STRAWBERRY_GENIUSLYRICSPROVIDER_H
#define STRAWBERRY_GENIUSLYRICSPROVIDER_H

#include "lyrics/htmllyricsprovider.h"

class GeniusLyricsProvider : public HtmlLyricsProvider {
 public:
  static const char *kApiUrl;
  static const char *kAuthUrl;

  GeniusLyricsProvider();
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Authenticate(const std::string &username, const std::string &token) override;
  void Logout() override;
  bool authenticated() const override { return !access_token_.empty(); }
  std::string username() const override { return username_; }

  static std::string SearchApiUrl(const std::string &query);
  static std::string AuthorizationUrl(const std::string &client_id, const std::string &redirect_uri);
  static std::string ParseSearchResultUrl(const std::string &json);

 protected:
  std::string UrlFor(const Song &song) const override;

 private:
  void FetchPage(const std::string &page_url, NetworkAccessManager *network, Callback callback);
  void SaveSession() const;

  std::string access_token_;
  std::string username_;
};

#endif
