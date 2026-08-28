#ifndef STRAWBERRY_STREAMINGSERVICE_H
#define STRAWBERRY_STREAMINGSERVICE_H

#include "core/signal.h"
#include "core/song.h"
#include "core/urlhandler.h"
#include "streaming/streamingpage.h"

#include <functional>
#include <map>
#include <string>

class NetworkAccessManager;

class StreamingService : public UrlHandler {
 public:
  using SearchCallback = std::function<void(const SongList &)>;
  enum class FavoriteType { Artists = 1, Albums = 2, Songs = 3 };
  enum class SearchType { Artists = 1, Albums = 2, Songs = 3 };

  virtual std::string name() const = 0;
  virtual NetworkAccessManager *network() const { return nullptr; }
  virtual void Search(const std::string &query, SearchCallback callback) = 0;
  virtual void Search(const std::string &query, SearchType type, SearchCallback callback);
  virtual void GetArtists(SearchCallback callback);
  virtual void GetAlbums(SearchCallback callback);
  virtual void GetSongs(SearchCallback callback);
  virtual void GetArtistAlbums(const Song &artist, SearchCallback callback);
  virtual void GetAlbumSongs(const Song &album, SearchCallback callback);
  virtual void Login(const std::string &username, const std::string &password_or_token) = 0;
  virtual void Logout();
  virtual void ReloadSettings() {}
  // Cached rows keep the bare cover art id, because the signed URL carries the user's credentials.  Services
  // that sign their cover URLs rebuild them here from the credentials currently configured.
  virtual SongList WithCoverUrls(SongList songs) const { return songs; }
  virtual bool logged_in() const { return logged_in_; }
  virtual bool authenticated() const { return logged_in_; }
  virtual bool show_progress() const { return true; }
  void NotifyAuthenticationChanged();
  void NotifyAuthenticationFailed(const std::string &error);
  int last_search_id() const { return last_search_id_; }
  int StartSearchProgress();
  void StartArtistsProgress();
  void StartAlbumsProgress();
  void StartSongsProgress();
  void StartFavoritesProgress(FavoriteType type = FavoriteType::Songs);
  virtual void CancelSearch();
  virtual void ResetArtistsRequest();
  virtual void ResetAlbumsRequest();
  virtual void ResetSongsRequest();
  virtual void ResetFavoritesRequest();
  int BeginArtistsRequest();
  int BeginAlbumsRequest();
  int BeginSongsRequest();
  int BeginSearchRequest();
  int BeginFavoritesRequest();
  bool ArtistsRequestCurrent(int generation) const;
  bool AlbumsRequestCurrent(int generation) const;
  bool SongsRequestCurrent(int generation) const;
  bool SearchRequestCurrent(int generation) const;
  bool FavoritesRequestCurrent(int generation) const;
  int artists_generation() const { return artists_gen_; }
  int albums_generation() const { return albums_gen_; }
  int songs_generation() const { return songs_gen_; }
  int search_generation() const { return search_gen_; }
  int favorites_generation() const { return favorites_gen_; }
  SearchCallback GuardArtists(SearchCallback callback);
  SearchCallback GuardAlbums(SearchCallback callback);
  SearchCallback GuardSongs(SearchCallback callback);
  SearchCallback GuardSearch(SearchCallback callback);
  SearchCallback GuardFavorites(SearchCallback callback);
  void ReportSearchProgress(int received, int total);
  void ReportArtistsProgress(int received, int total);
  void ReportAlbumsProgress(int received, int total);
  void ReportSongsProgress(int received, int total);
  void ReportFavoritesProgress(int received, int total);
  void NotifySearchFailed(const std::string &error);
  void NotifyArtistsFailed(const std::string &error);
  void NotifyAlbumsFailed(const std::string &error);
  void NotifySongsFailed(const std::string &error);
  void NotifyFavoritesFailed(const std::string &error);
  void DeliverWithCovers(NetworkAccessManager *network, const std::map<std::string, std::string> &headers, const SongList &songs,
                         SearchCallback callback, std::function<void(const std::string &)> status = {},
                         StreamingPage::ProgressCallback progress = {}, StreamingPage::StillCurrent still_current = {});
  virtual void GetFavorites(FavoriteType type, SearchCallback callback);
  virtual void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {});
  virtual void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {});

  Signal<> AuthenticationChanged;
  Signal<std::string> AuthenticationFailed;
  Signal<int, std::string> SearchUpdateStatus;
  Signal<int, int> SearchProgressSetMaximum;
  Signal<int, int> SearchUpdateProgress;
  Signal<std::string> ArtistsUpdateStatus;
  Signal<int> ArtistsProgressSetMaximum;
  Signal<int> ArtistsUpdateProgress;
  Signal<std::string> AlbumsUpdateStatus;
  Signal<int> AlbumsProgressSetMaximum;
  Signal<int> AlbumsUpdateProgress;
  Signal<std::string> SongsUpdateStatus;
  Signal<int> SongsProgressSetMaximum;
  Signal<int> SongsUpdateProgress;
  Signal<std::string> FavoritesUpdateStatus;
  Signal<int> FavoritesProgressSetMaximum;
  Signal<int> FavoritesUpdateProgress;
  Signal<int, std::string> SearchFailed;
  Signal<std::string> ArtistsFailed;
  Signal<std::string> AlbumsFailed;
  Signal<std::string> SongsFailed;
  Signal<std::string> FavoritesFailed;

 protected:
  bool logged_in_ = false;
  int last_search_id_ = 0;
  int artists_gen_ = 0;
  int albums_gen_ = 0;
  int songs_gen_ = 0;
  int search_gen_ = 0;
  int favorites_gen_ = 0;
};

#endif
