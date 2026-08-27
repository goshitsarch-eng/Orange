#include "lyrics/geniuslyricsprovider.h"

#include "core/oauthenticator.h"
#include "core/settings.h"
#include "lyrics/geniuslyricscredentials.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

const char *GeniusLyricsProvider::kApiUrl = "https://api.genius.com";
const char *GeniusLyricsProvider::kAuthUrl = "https://api.genius.com/oauth/authorize";

GeniusLyricsProvider::GeniusLyricsProvider()
    : HtmlLyricsProvider("Genius", "<div[^>]*>", "</div>", "data-lyrics-container", true) {
  Settings settings;
  settings.BeginGroup("Genius");
  access_token_ = settings.Value("access_token");
  username_ = settings.Value("username");
}

std::string GeniusLyricsProvider::UrlFor(const Song &song) const {
  return "https://genius.com/" + SlugDashed(song.artist()) + "-" + SlugDashed(song.title()) + "-lyrics";
}

std::string GeniusLyricsProvider::SearchApiUrl(const std::string &query) {
  return std::string(kApiUrl) + "/search?q=" + StrUtils::UriEscape(query);
}

std::string GeniusLyricsProvider::AuthorizationUrl(const std::string &client_id, const std::string &redirect_uri) {
  return std::string(kAuthUrl) + "?client_id=" + StrUtils::UriEscape(client_id) +
         "&redirect_uri=" + StrUtils::UriEscape(redirect_uri) + "&scope=me&response_type=code";
}

std::string GeniusLyricsProvider::ParseSearchResultUrl(const std::string &json) {
  const std::string url = JsonUtils::FindStringByKeys(json, {"url"});
  if (url.find("genius.com") != std::string::npos) {
    return url;
  }
  const std::string path = JsonUtils::FindStringByKeys(json, {"path"});
  if (StrUtils::StartsWith(path, "/")) {
    return "https://genius.com" + path;
  }
  return url;
}

void GeniusLyricsProvider::SaveSession() const {
  Settings settings;
  settings.BeginGroup("Genius");
  settings.SetValue("access_token", access_token_);
  settings.SetValue("username", username_);
  settings.Sync();
}

void GeniusLyricsProvider::Authenticate(const std::string &username, const std::string &token) {
  username_ = username;
  access_token_ = token;
  SaveSession();
}

void GeniusLyricsProvider::Authenticate(NetworkAccessManager *network, std::function<void()> done) {
  if (!network) {
    if (done) {
      done();
    }
    return;
  }
  Settings settings;
  settings.BeginGroup("Genius");
  const std::string client_id = GeniusLyricsCredentials::EffectiveClientId(settings.Value("client_id"));
  const std::string client_secret = GeniusLyricsCredentials::EffectiveClientSecret(settings.Value("client_secret"));
  auto *oauth = new OAuthenticator(network);
  oauth->AuthorizeInBrowser(kAuthUrl, client_id, GeniusLyricsCredentials::kScope,
                            [this, oauth, client_id, client_secret, done](const std::string &code, const std::string &) {
                              if (code.empty()) {
                                delete oauth;
                                if (done) {
                                  done();
                                }
                                return;
                              }
                              oauth->ExchangeCode(GeniusLyricsCredentials::kTokenUrl, client_id, client_secret, code,
                                                  [this, oauth, done](const std::string &body, const std::string &) {
                                                    const auto tokens = OAuthenticator::ParseTokenResponse(body);
                                                    if (!tokens.access_token.empty()) {
                                                      Authenticate({}, tokens.access_token);
                                                    }
                                                    delete oauth;
                                                    if (done) {
                                                      done();
                                                    }
                                                  });
                            },
                            static_cast<guint16>(kOAuthPort));
}

void GeniusLyricsProvider::Logout() {
  username_.clear();
  access_token_.clear();
  SaveSession();
}

void GeniusLyricsProvider::FetchPage(const std::string &page_url, NetworkAccessManager *network, Callback callback) {
  network->Get(page_url, [this, callback](const NetworkAccessManager::Response &page_response) {
    if (!page_response.ok()) {
      callback({}, page_response.error.empty() ? "Genius lyrics request failed" : page_response.error);
      return;
    }
    const std::string lyrics = ParseLyricsFromHTML(page_response.body, start_tag_, end_tag_, lyrics_start_, multiple_);
    if (lyrics.empty()) {
      callback({}, "No lyrics in Genius response");
      return;
    }
    callback(lyrics, {});
  });
}

void GeniusLyricsProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.artist().empty() || song.title().empty()) {
    callback({}, "No artist or title");
    return;
  }
  const std::string query = song.artist() + " " + song.title();
  if (!access_token_.empty()) {
    network->Get(SearchApiUrl(query),
                 [this, song, network, callback](const NetworkAccessManager::Response &search_response) {
                   if (!search_response.ok()) {
                     FetchPage(UrlFor(song), network, callback);
                     return;
                   }
                   const std::string url = ParseSearchResultUrl(search_response.body);
                   if (url.empty()) {
                     FetchPage(UrlFor(song), network, callback);
                     return;
                   }
                   FetchPage(url, network, callback);
                 },
                 {{"Authorization", "Bearer " + access_token_}});
    return;
  }
  FetchPage(UrlFor(song), network, [this, song, network, callback](const std::string &lyrics, const std::string &error) {
    if (!lyrics.empty()) {
      callback(lyrics, {});
      return;
    }
    (void)error;
    const std::string search = "https://genius.com/api/search/song?q=" + StrUtils::UriEscape(song.artist() + " " + song.title());
    network->Get(search, [this, network, callback](const NetworkAccessManager::Response &search_response) {
      if (!search_response.ok()) {
        callback({}, search_response.error.empty() ? "Genius search failed" : search_response.error);
        return;
      }
      const std::string url = ParseSearchResultUrl(search_response.body);
      if (url.empty()) {
        callback({}, "No Genius result");
        return;
      }
      FetchPage(url, network, callback);
    });
  });
}
