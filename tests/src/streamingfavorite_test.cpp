#include "qobuz/qobuzfavoriterequest.h"
#include "spotify/spotifyfavoriterequest.h"
#include "subsonic/subsonicfavoriterequest.h"
#include "subsonic/subsonicservice.h"
#include "tidal/tidalfavoriterequest.h"
#include "utilities/jsonutils.h"

#include <gtest/gtest.h>

TEST(TidalFavoriteRequest, TextMethodAndUrls) {
  using Type = StreamingService::FavoriteType;
  EXPECT_EQ("artists", TidalFavoriteRequest::FavoriteText(Type::Artists));
  EXPECT_EQ("albums", TidalFavoriteRequest::FavoriteText(Type::Albums));
  EXPECT_EQ("tracks", TidalFavoriteRequest::FavoriteText(Type::Songs));
  EXPECT_EQ("artistIds", TidalFavoriteRequest::FavoriteMethod(Type::Artists));
  EXPECT_EQ("albumIds", TidalFavoriteRequest::FavoriteMethod(Type::Albums));
  EXPECT_EQ("trackIds", TidalFavoriteRequest::FavoriteMethod(Type::Songs));

  EXPECT_EQ("https://api.tidalhifi.com/v1/users/42/favorites/tracks?countryCode=US&limit=50",
            TidalFavoriteRequest::ListUrl("https://api.tidalhifi.com/v1", 42, Type::Songs, "US"));
  EXPECT_EQ("https://api.tidalhifi.com/v1/users/42/favorites/tracks",
            TidalFavoriteRequest::AddUrl("https://api.tidalhifi.com/v1", 42, Type::Songs));
  EXPECT_EQ("countryCode=US&trackIds=99%2C100", TidalFavoriteRequest::AddFormBody(Type::Songs, "US", {"99", "100"}));
  EXPECT_EQ("https://api.tidalhifi.com/v1/users/42/favorites/tracks/99?countryCode=US",
            TidalFavoriteRequest::RemoveUrl("https://api.tidalhifi.com/v1", 42, Type::Songs, "99", "US"));
}

TEST(TidalFavoriteRequest, IdsFromSongsAndWrappedJson) {
  Song song(Song::Source::Tidal);
  song.set_song_id("99");
  song.set_artist_id("7");
  song.set_album_id("8");
  EXPECT_EQ(std::vector<std::string>({"99"}), TidalFavoriteRequest::IdsFromSongs(StreamingService::FavoriteType::Songs, {song}));
  EXPECT_EQ(std::vector<std::string>({"7"}), TidalFavoriteRequest::IdsFromSongs(StreamingService::FavoriteType::Artists, {song}));

  const std::string json = R"json({"items":[{"item":{"id":99,"title":"Helplessness Blues","artist":{"id":7,"name":"Fleet Foxes"},"album":{"id":8,"title":"Helplessness Blues"},"duration":301}}]})json";
  const SongList songs = JsonUtils::ParseTidalTracks(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Helplessness Blues", songs.front().title());
  EXPECT_EQ("Fleet Foxes", songs.front().artist());
  EXPECT_EQ("99", songs.front().song_id());
  EXPECT_EQ("7", songs.front().artist_id());
  EXPECT_EQ("8", songs.front().album_id());
}

TEST(SpotifyFavoriteRequest, UrlsAndJsonArray) {
  using Type = StreamingService::FavoriteType;
  EXPECT_EQ("https://api.spotify.com/v1/me/tracks?limit=50", SpotifyFavoriteRequest::ListUrl("https://api.spotify.com/v1", Type::Songs));
  EXPECT_EQ("https://api.spotify.com/v1/me/following?type=artist&limit=50",
            SpotifyFavoriteRequest::ListUrl("https://api.spotify.com/v1", Type::Artists));
  EXPECT_EQ("https://api.spotify.com/v1/me/following?type=artist&ids=a%2Cb",
            SpotifyFavoriteRequest::MutateUrl("https://api.spotify.com/v1", Type::Artists, {"a", "b"}));
  EXPECT_EQ("[\"abc\",\"def\"]", SpotifyFavoriteRequest::JsonIdArray({"abc", "def"}));

  const std::string json = R"json({"items":[{"added_at":"2020-01-01","track":{"id":"abc","name":"Song","artists":[{"id":"art","name":"Artist"}],"album":{"id":"alb","name":"Album"},"duration_ms":123000}}]})json";
  const SongList songs = JsonUtils::ParseSpotifyTracks(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Song", songs.front().title());
  EXPECT_EQ("abc", songs.front().song_id());
  EXPECT_EQ("art", songs.front().artist_id());
  EXPECT_EQ("alb", songs.front().album_id());
}

TEST(QobuzFavoriteRequest, CreateDeleteAndListUrls) {
  using Type = StreamingService::FavoriteType;
  EXPECT_EQ("artist_ids", QobuzFavoriteRequest::FavoriteMethod(Type::Artists));
  EXPECT_EQ("album_ids", QobuzFavoriteRequest::FavoriteMethod(Type::Albums));
  EXPECT_EQ("track_ids", QobuzFavoriteRequest::FavoriteMethod(Type::Songs));
  const std::string list = QobuzFavoriteRequest::ListUrl("https://www.qobuz.com/api.json/0.2", Type::Songs, "app", "tok");
  EXPECT_NE(std::string::npos, list.find("/favorite/getUserFavorites?type=tracks"));
  EXPECT_NE(std::string::npos, list.find("app_id=app"));
  EXPECT_NE(std::string::npos, list.find("user_auth_token=tok"));
  const std::string create = QobuzFavoriteRequest::CreateUrl("https://www.qobuz.com/api.json/0.2", Type::Songs, {"7", "8"}, "app", "tok");
  EXPECT_NE(std::string::npos, create.find("/favorite/create?track_ids=7%2C8"));
  const std::string del = QobuzFavoriteRequest::DeleteUrl("https://www.qobuz.com/api.json/0.2", Type::Albums, {"9"}, "app", "tok");
  EXPECT_NE(std::string::npos, del.find("/favorite/delete?album_ids=9"));
}

TEST(SubsonicFavoriteRequest, StarParamsAndStarredJson) {
  using Type = StreamingService::FavoriteType;
  EXPECT_EQ("id", SubsonicFavoriteRequest::IdParam(Type::Songs));
  EXPECT_EQ("albumId", SubsonicFavoriteRequest::IdParam(Type::Albums));
  EXPECT_EQ("artistId", SubsonicFavoriteRequest::IdParam(Type::Artists));
  EXPECT_EQ("star", SubsonicFavoriteRequest::StarResource(false));
  EXPECT_EQ("unstar", SubsonicFavoriteRequest::StarResource(true));
  const auto params = SubsonicFavoriteRequest::StarParams(Type::Songs, "42");
  ASSERT_EQ("42", params.at("id"));

  const std::string url = SubsonicService::CreateUrl("https://music.example.com", "alice", "secret", "star", {{"id", "42"}}, true);
  EXPECT_NE(std::string::npos, url.find("/rest/star.view"));
  EXPECT_NE(std::string::npos, url.find("id=42"));
  EXPECT_NE(std::string::npos, url.find("p=enc:"));

  const std::string json = R"json({"subsonic-response":{"starred2":{"song":[{"id":"42","title":"Helplessness Blues","artist":"Fleet Foxes","artistId":"a1","albumId":"b1"}]}}})json";
  const SongList songs = JsonUtils::ParseSubsonicSongs(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Helplessness Blues", songs.front().title());
  EXPECT_EQ("42", songs.front().song_id());
  EXPECT_EQ("a1", songs.front().artist_id());
  EXPECT_EQ("b1", songs.front().album_id());
}
