#ifndef STRAWBERRY_OPENTIDALCOVERPROVIDER_H
#define STRAWBERRY_OPENTIDALCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <glib.h>

#include <string>
#include <vector>

class OpenTidalCoverProvider : public CoverProvider {
 public:
  struct AlbumHit {
    std::string id;
    std::string title;
  };

  struct ArtworkFile {
    std::string href;
    int width = 0;
    int height = 0;
  };

  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
    int width = 0;
    int height = 0;
  };

  struct Token {
    std::string access_token;
    std::string token_type;
    gint64 expires_in = 0;
  };

  struct ApiError {
    std::string category;
    std::string code;
    std::string detail;
    bool authentication_error = false;
  };

  static const char *kSettingsGroup;
  static const char *kOAuthAccessTokenUrl;
  static const char *kApiUrl;
  static const char *kApiClientIdB64;
  static const char *kApiClientSecretB64;
  static const char *kContentTypeHeader;
  static const int kSearchLimit;
  static const int kMinImageSize;

  std::string name() const override { return "OpenTidal"; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string ClientId();
  static std::string ClientSecret();
  static std::string SearchQuery(const std::string &artist, const std::string &album, const std::string &title);
  static std::string SearchUrl(const std::string &artist, const std::string &album, const std::string &title, const std::string &country = "US");
  static std::string CoverArtUrl(const std::string &album_id, const std::string &country = "US");
  static std::string ArtworkUrl(const std::string &artwork_id, const std::string &country = "US");
  static std::string AuthorizationHeader(const std::string &access_token);

  static std::vector<AlbumHit> ParseSearchAlbums(const std::string &json);
  static std::vector<std::string> ParseCoverArtIds(const std::string &json);
  static std::vector<ArtworkFile> ParseArtworkFiles(const std::string &json);
  static std::vector<SearchResult> ResultsFromFiles(const std::string &artist, const std::string &album, const std::vector<ArtworkFile> &files);
  static Token ParseToken(const std::string &json);
  static ApiError ParseApiError(const std::string &json);
  static bool AcceptImage(int width, int height);

 private:
  void EnsureToken(NetworkAccessManager *network, const std::function<void(const std::string &token, const std::string &error)> &callback);
  void SearchAlbums(NetworkAccessManager *network, const std::string &token, const Song &song, Callback callback);
  void FetchAlbumCovers(NetworkAccessManager *network, const std::string &token, const std::string &artist, std::vector<AlbumHit> albums,
                        Callback callback);
  void FetchArtworks(NetworkAccessManager *network, const std::string &token, const std::string &artist, const AlbumHit &album,
                     std::vector<std::string> artwork_ids, std::vector<AlbumHit> remaining, Callback callback);
  void SearchAlbums(NetworkAccessManager *network, const std::string &token, const Song &song, SearchCallback callback);
  void SearchAlbumCovers(NetworkAccessManager *network, const std::string &token, const std::string &artist, std::vector<AlbumHit> albums,
                         SearchCallback callback, CoverProviderSearchResults collected = {});
  void SearchArtworks(NetworkAccessManager *network, const std::string &token, const std::string &artist, const AlbumHit &album,
                      std::vector<std::string> artwork_ids, std::vector<AlbumHit> remaining, SearchCallback callback,
                      CoverProviderSearchResults collected);
};

#endif
