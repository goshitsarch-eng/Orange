#include "qobuz/qobuzrequest.h"
#include "spotify/spotifyrequest.h"
#include "subsonic/subsonicrequest.h"
#include "tidal/tidalrequest.h"
#include "utilities/jsonutils.h"

#include <gtest/gtest.h>

TEST(TidalRequest, UrlsMatchOriginalResources) {
  EXPECT_EQ("users/42/favorites/artists", TidalRequest::Resource(TidalRequest::Type::FavouriteArtists, 42));
  EXPECT_EQ("search/albums", TidalRequest::Resource(TidalRequest::Type::SearchAlbums, 0));
  EXPECT_EQ("search/tracks", TidalRequest::Resource(TidalRequest::Type::SearchSongs, 0));
  const std::string search = TidalRequest::Url("https://api.tidalhifi.com/v1", TidalRequest::Type::SearchArtists, "foxes", "US");
  EXPECT_NE(std::string::npos, search.find("/search/artists?"));
  EXPECT_NE(std::string::npos, search.find("query=foxes"));
  EXPECT_NE(std::string::npos, search.find("countryCode=US"));
  EXPECT_NE(std::string::npos, search.find("limit=50"));
  EXPECT_EQ("https://api.tidalhifi.com/v1/artists/7/albums?countryCode=US&limit=50",
            TidalRequest::ArtistAlbumsUrl("https://api.tidalhifi.com/v1", "7", "US"));
  EXPECT_EQ("https://api.tidalhifi.com/v1/albums/8/tracks?countryCode=US&limit=50",
            TidalRequest::AlbumSongsUrl("https://api.tidalhifi.com/v1", "8", "US"));
  EXPECT_EQ("https://resources.tidal.com/images/aa/bb/cc/1280x1280.jpg", TidalRequest::CoverUrl("aa-bb-cc"));
  EXPECT_EQ(TidalRequest::Type::SearchAlbums, TidalRequest::FromSearchType(StreamingService::SearchType::Albums));
}

TEST(TidalRequest, ParsesArtistsAlbumsAndTracks) {
  const SongList artists = TidalRequest::Parse(TidalRequest::Type::SearchArtists, R"json({"items":[{"id":7,"name":"Fleet Foxes"}]})json");
  ASSERT_EQ(1u, artists.size());
  EXPECT_EQ("Fleet Foxes", artists.front().artist());
  EXPECT_EQ("7", artists.front().artist_id());
  EXPECT_EQ("tidal://artist/7", artists.front().url());

  const SongList albums = TidalRequest::Parse(
      TidalRequest::Type::FavouriteAlbums,
      R"json({"items":[{"item":{"id":8,"title":"Helplessness Blues","artist":{"id":7,"name":"Fleet Foxes"},"cover":"aa-bb"}}]})json");
  ASSERT_EQ(1u, albums.size());
  EXPECT_EQ("Helplessness Blues", albums.front().album());
  EXPECT_EQ("8", albums.front().album_id());
  EXPECT_NE(std::string::npos, albums.front().art_automatic().find("/images/aa/bb/1280x1280.jpg"));

  const SongList songs = TidalRequest::Parse(
      TidalRequest::Type::SearchSongs,
      R"json({"items":[{"id":99,"title":"Lorelai","artist":{"id":7,"name":"Fleet Foxes"},"album":{"id":8,"title":"Helplessness Blues"},"duration":270}]})json");
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Lorelai", songs.front().title());
  EXPECT_EQ("99", songs.front().song_id());
}

TEST(SpotifyRequest, UrlsAndSearchType) {
  EXPECT_EQ("artist", SpotifyRequest::SearchTypeParam(SpotifyRequest::Type::SearchArtists));
  EXPECT_EQ("album", SpotifyRequest::SearchTypeParam(SpotifyRequest::Type::SearchAlbums));
  EXPECT_EQ("track", SpotifyRequest::SearchTypeParam(SpotifyRequest::Type::SearchSongs));
  EXPECT_EQ("https://api.spotify.com/v1/me/following?type=artist&limit=50",
            SpotifyRequest::Url("https://api.spotify.com/v1", SpotifyRequest::Type::FavouriteArtists, {}));
  EXPECT_EQ("https://api.spotify.com/v1/me/albums?limit=50",
            SpotifyRequest::Url("https://api.spotify.com/v1", SpotifyRequest::Type::FavouriteAlbums, {}));
  const std::string search = SpotifyRequest::Url("https://api.spotify.com/v1", SpotifyRequest::Type::SearchAlbums, "dummy");
  EXPECT_NE(std::string::npos, search.find("/search?type=album"));
  EXPECT_NE(std::string::npos, search.find("q=dummy"));
  EXPECT_EQ("https://api.spotify.com/v1/artists/a1/albums?limit=50", SpotifyRequest::ArtistAlbumsUrl("https://api.spotify.com/v1", "a1"));
  EXPECT_EQ("https://api.spotify.com/v1/albums/b2/tracks?limit=50", SpotifyRequest::AlbumSongsUrl("https://api.spotify.com/v1", "b2"));
}

TEST(SpotifyRequest, ParsesArtistsAndAlbums) {
  const SongList artists = SpotifyRequest::Parse(
      SpotifyRequest::Type::SearchArtists, R"json({"artists":{"items":[{"id":"art","name":"Portishead","images":[{"url":"https://i/a.jpg"}]}]}})json");
  ASSERT_EQ(1u, artists.size());
  EXPECT_EQ("Portishead", artists.front().artist());
  EXPECT_EQ("art", artists.front().artist_id());
  EXPECT_EQ("https://i/a.jpg", artists.front().art_automatic());

  const SongList albums = SpotifyRequest::Parse(
      SpotifyRequest::Type::FavouriteAlbums,
      R"json({"items":[{"album":{"id":"alb","name":"Dummy","artists":[{"id":"art","name":"Portishead"}]}}]})json");
  ASSERT_EQ(1u, albums.size());
  EXPECT_EQ("Dummy", albums.front().album());
  EXPECT_EQ("alb", albums.front().album_id());
  EXPECT_EQ("Portishead", albums.front().artist());
}

TEST(QobuzRequest, UrlsAndParse) {
  EXPECT_EQ("artist/search", QobuzRequest::Resource(QobuzRequest::Type::SearchArtists));
  EXPECT_EQ("album/search", QobuzRequest::Resource(QobuzRequest::Type::SearchAlbums));
  EXPECT_EQ("track/search", QobuzRequest::Resource(QobuzRequest::Type::SearchSongs));
  EXPECT_EQ("favorite/getUserFavorites", QobuzRequest::Resource(QobuzRequest::Type::FavouriteAlbums));
  const std::string fav = QobuzRequest::Url("https://www.qobuz.com/api.json/0.2", QobuzRequest::Type::FavouriteArtists, {}, "app", "tok");
  EXPECT_NE(std::string::npos, fav.find("/favorite/getUserFavorites?type=artists"));
  EXPECT_NE(std::string::npos, fav.find("app_id=app"));
  EXPECT_NE(std::string::npos, fav.find("user_auth_token=tok"));
  const std::string search = QobuzRequest::Url("https://www.qobuz.com/api.json/0.2", QobuzRequest::Type::SearchSongs, "roads", "app", "tok");
  EXPECT_NE(std::string::npos, search.find("/track/search?query=roads"));
  EXPECT_NE(std::string::npos, QobuzRequest::ArtistAlbumsUrl("https://www.qobuz.com/api.json/0.2", "7", "app", "tok").find("/artist/get?artist_id=7"));
  EXPECT_NE(std::string::npos, QobuzRequest::AlbumSongsUrl("https://www.qobuz.com/api.json/0.2", "8", "app", "tok").find("/album/get?album_id=8"));

  const SongList artists =
      QobuzRequest::Parse(QobuzRequest::Type::SearchArtists, R"json({"artists":{"items":[{"id":7,"name":"Portishead"}]}})json");
  ASSERT_EQ(1u, artists.size());
  EXPECT_EQ("7", artists.front().artist_id());
  const SongList albums = QobuzRequest::Parse(
      QobuzRequest::Type::SearchAlbums,
      R"json({"albums":{"items":[{"id":8,"title":"Dummy","artist":{"id":7,"name":"Portishead"},"image":{"large":"https://i/l.jpg"}}]}})json");
  ASSERT_EQ(1u, albums.size());
  EXPECT_EQ("Dummy", albums.front().album());
  EXPECT_EQ("https://i/l.jpg", albums.front().art_automatic());
}

TEST(SubsonicRequest, ResourcesParamsAndParse) {
  EXPECT_EQ("search3", SubsonicRequest::Resource(SubsonicRequest::Type::SearchSongs));
  EXPECT_EQ("getAlbumList2", SubsonicRequest::Resource(SubsonicRequest::Type::AlbumList));
  EXPECT_EQ("getArtists", SubsonicRequest::Resource(SubsonicRequest::Type::ArtistsList));
  EXPECT_EQ("getStarred2", SubsonicRequest::Resource(SubsonicRequest::Type::FavouriteSongs));
  const auto search = SubsonicRequest::Params(SubsonicRequest::Type::SearchArtists, "foxes");
  EXPECT_EQ("foxes", search.at("query"));
  EXPECT_EQ("50", search.at("artistCount"));
  EXPECT_EQ("0", search.at("songCount"));
  const auto albums = SubsonicRequest::Params(SubsonicRequest::Type::AlbumList, {});
  EXPECT_EQ("alphabeticalByName", albums.at("type"));
  EXPECT_EQ("50", albums.at("size"));
  EXPECT_EQ("12", SubsonicRequest::AlbumSongsParams("12").at("id"));

  const SongList artists = SubsonicRequest::Parse(
      SubsonicRequest::Type::ArtistsList,
      R"json({"subsonic-response":{"artists":{"index":[{"name":"F","artist":[{"id":"1","name":"Fleet Foxes"}]}]}}})json");
  ASSERT_EQ(1u, artists.size());
  EXPECT_EQ("Fleet Foxes", artists.front().artist());
  EXPECT_EQ("1", artists.front().artist_id());

  const SongList album_list = SubsonicRequest::Parse(
      SubsonicRequest::Type::AlbumList,
      R"json({"subsonic-response":{"albumList2":{"album":[{"id":"8","name":"Helplessness Blues","artist":"Fleet Foxes","artistId":"1","year":2011}]}}})json");
  ASSERT_EQ(1u, album_list.size());
  EXPECT_EQ("Helplessness Blues", album_list.front().album());
  EXPECT_EQ("8", album_list.front().album_id());
  EXPECT_EQ(2011, album_list.front().year());
}
