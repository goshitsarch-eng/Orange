#include "qobuz/qobuzfavoriterequest.h"
#include "spotify/spotifyfavoriterequest.h"
#include "streaming/streamingbrowse.h"
#include "streaming/streamingfavoriteaction.h"
#include "subsonic/subsonicfavoriterequest.h"
#include "subsonic/subsonicservice.h"
#include "tidal/tidalfavoriterequest.h"
#include "utilities/jsonutils.h"

#include <gtest/gtest.h>

TEST(StreamingFavoriteAction, TypeAndStoreHelpers) {
  using Type = StreamingService::FavoriteType;
  EXPECT_EQ(Type::Artists, StreamingFavoriteAction::FromInt(1));
  EXPECT_EQ(Type::Albums, StreamingFavoriteAction::FromInt(2));
  EXPECT_EQ(Type::Songs, StreamingFavoriteAction::FromInt(3));
  EXPECT_EQ(Type::Songs, StreamingFavoriteAction::FromInt(0));
  EXPECT_EQ(1, StreamingFavoriteAction::ToInt(Type::Artists));
  EXPECT_EQ(StreamingCollectionStore::List::Artists, StreamingFavoriteAction::StoreList(Type::Artists));
  EXPECT_EQ(StreamingCollectionStore::List::Albums, StreamingFavoriteAction::StoreList(Type::Albums));
  EXPECT_EQ(StreamingCollectionStore::List::Songs, StreamingFavoriteAction::StoreList(Type::Songs));
  EXPECT_EQ(Type::Artists, StreamingFavoriteAction::TypeFromList(StreamingCollectionStore::List::Artists));
  EXPECT_EQ(Type::Albums, StreamingFavoriteAction::TypeFromList(StreamingCollectionStore::List::Albums));
  EXPECT_EQ(Type::Songs, StreamingFavoriteAction::TypeFromList(StreamingCollectionStore::List::Songs));
  EXPECT_STREQ("Artists", StreamingFavoriteAction::Label(Type::Artists));
  EXPECT_STREQ("Receiving albums...", StreamingFavoriteAction::Receiving(Type::Albums));
  EXPECT_STREQ("No favorite artists", StreamingFavoriteAction::EmptyStatus(Type::Artists, true));
  EXPECT_STREQ("Sign in in Preferences", StreamingFavoriteAction::EmptyStatus(Type::Songs, false));
}

TEST(StreamingFavoriteAction, TypeForSongs) {
  Song track(Song::Source::Tidal);
  track.set_song_id("99");
  track.set_album_id("8");
  track.set_artist_id("7");
  EXPECT_EQ(StreamingService::FavoriteType::Songs, StreamingFavoriteAction::TypeForSongs({track}));
  Song album(Song::Source::Tidal);
  album.set_album_id("8");
  album.set_artist_id("7");
  EXPECT_EQ(StreamingService::FavoriteType::Albums, StreamingFavoriteAction::TypeForSongs({album}));
  Song artist(Song::Source::Tidal);
  artist.set_artist_id("7");
  EXPECT_EQ(StreamingService::FavoriteType::Artists, StreamingFavoriteAction::TypeForSongs({artist}));
}

TEST(StreamingBrowse, KindFromIds) {
  Song track;
  track.set_song_id("99");
  track.set_album_id("8");
  EXPECT_EQ(StreamingBrowse::Kind::Song, StreamingBrowse::KindOf(track));
  EXPECT_FALSE(StreamingBrowse::CanBrowse(StreamingBrowse::KindOf(track)));
  Song album;
  album.set_album_id("8");
  album.set_title("Dummy");
  EXPECT_EQ(StreamingBrowse::Kind::Album, StreamingBrowse::KindOf(album));
  EXPECT_TRUE(StreamingBrowse::CanBrowse(StreamingBrowse::KindOf(album)));
  Song artist;
  artist.set_artist_id("7");
  artist.set_title("Portishead");
  EXPECT_EQ(StreamingBrowse::Kind::Artist, StreamingBrowse::KindOf(artist));
}

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
  EXPECT_NE(std::string::npos, TidalFavoriteRequest::ListUrl("https://api.tidalhifi.com/v1", 42, Type::Songs, "US", 50, 50).find("offset=50"));
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
  const SongList parsed_songs = TidalFavoriteRequest::Parse(StreamingService::FavoriteType::Songs, json);
  ASSERT_EQ(1u, parsed_songs.size());
  EXPECT_EQ("99", parsed_songs.front().song_id());
  const SongList artists = TidalFavoriteRequest::Parse(StreamingService::FavoriteType::Artists, R"json({"items":[{"id":7,"name":"Fleet Foxes"}]})json");
  ASSERT_EQ(1u, artists.size());
  EXPECT_EQ("7", artists.front().artist_id());
}

TEST(SpotifyFavoriteRequest, UrlsAndJsonArray) {
  using Type = StreamingService::FavoriteType;
  EXPECT_EQ("https://api.spotify.com/v1/me/tracks?limit=50", SpotifyFavoriteRequest::ListUrl("https://api.spotify.com/v1", Type::Songs));
  EXPECT_EQ("https://api.spotify.com/v1/me/following?type=artist&limit=50",
            SpotifyFavoriteRequest::ListUrl("https://api.spotify.com/v1", Type::Artists));
  EXPECT_NE(std::string::npos, SpotifyFavoriteRequest::ListUrl("https://api.spotify.com/v1", Type::Songs, 50, 50).find("offset=50"));
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
  const SongList parsed = SpotifyFavoriteRequest::Parse(Type::Songs, json);
  ASSERT_EQ(1u, parsed.size());
  EXPECT_EQ("abc", parsed.front().song_id());
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
  EXPECT_NE(std::string::npos, QobuzFavoriteRequest::ListUrl("https://www.qobuz.com/api.json/0.2", Type::Songs, "app", "tok", 50, 50).find("offset=50"));
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
  const auto batch = SubsonicFavoriteRequest::StarParamsForIds(Type::Songs, {"42", "99"});
  ASSERT_EQ(2u, batch.size());
  EXPECT_EQ("42", batch[0].at("id"));
  EXPECT_EQ("99", batch[1].at("id"));

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
