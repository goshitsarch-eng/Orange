#include "scrobbler/scrobblereligibility.h"
#include "scrobbler/lastfmscrobbler.h"
#include "scrobbler/listenbrainzscrobbler.h"
#include "scrobbler/scrobblercache.h"
#include "scrobbler/subsonicscrobbler.h"

#include <gtest/gtest.h>

TEST(ScrobblerCache, RoundTripJson) {
  ScrobblerCacheItem item;
  item.timestamp = 1700000000;
  item.artist = "Portishead";
  item.album = "Dummy";
  item.title = "Roads";
  item.albumartist = "Portishead";
  item.track = 8;
  item.length_nanosec = 300000000000LL;
  const std::string json = ScrobblerCache::ToJson({item});
  const auto parsed = ScrobblerCache::Parse(json);
  ASSERT_EQ(1u, parsed.size());
  EXPECT_EQ(1700000000u, parsed.front().timestamp);
  EXPECT_EQ("Portishead", parsed.front().artist);
  EXPECT_EQ("Dummy", parsed.front().album);
  EXPECT_EQ("Roads", parsed.front().title);
  EXPECT_EQ(8, parsed.front().track);
  EXPECT_EQ(300000000000LL, parsed.front().length_nanosec);
}

TEST(ScrobblerCache, RejectsIncompleteTracks) {
  const std::string json = R"({"tracks":[{"timestamp":1,"artist":"","album":"A","title":"T","track":1,"albumartist":"","length_nanosec":1}]})";
  EXPECT_TRUE(ScrobblerCache::Parse(json).empty());
}

TEST(LastFmScrobbler, ScrobbleParamsUseIndexedKeys) {
  ScrobblerCacheItem item;
  item.timestamp = 42;
  item.artist = "Portishead";
  item.title = "Roads";
  item.album = "Dummy";
  item.length_nanosec = 5000000000LL;
  const auto params = LastFmScrobbler::ScrobbleParams({item}, "session");
  EXPECT_EQ("track.scrobble", params.at("method"));
  EXPECT_EQ("Portishead", params.at("artist[0]"));
  EXPECT_EQ("Roads", params.at("track[0]"));
  EXPECT_EQ("42", params.at("timestamp[0]"));
  EXPECT_EQ("Dummy", params.at("album[0]"));
  EXPECT_EQ("5", params.at("duration[0]"));
  EXPECT_EQ("session", params.at("sk"));
}

TEST(LastFmScrobbler, SignIsStable) {
  const std::map<std::string, std::string> params = {{"api_key", LastFmScrobbler::kApiKey}, {"method", "track.scrobble"}};
  EXPECT_EQ(32u, LastFmScrobbler::Sign(params).size());
  EXPECT_EQ(LastFmScrobbler::Sign(params), LastFmScrobbler::Sign(params));
}

TEST(ListenBrainzScrobbler, SubmitBodyEscapesQuotes) {
  ScrobblerCacheItem item;
  item.timestamp = 99;
  item.artist = "AC\"DC";
  item.title = "It's a Long Way";
  item.album = "High Voltage";
  const std::string body = ListenBrainzScrobbler::SubmitBody("single", {item});
  EXPECT_NE(std::string::npos, body.find("\"listen_type\":\"single\""));
  EXPECT_NE(std::string::npos, body.find("\"listened_at\":99"));
  EXPECT_NE(std::string::npos, body.find("AC\\\"DC"));
  EXPECT_EQ(std::string::npos, body.find("\"artist_name\":\"AC\"DC\""));
}

TEST(SubsonicScrobbler, ScrobbleUrlUsesRestEndpoint) {
  const std::string url = SubsonicScrobbler::ScrobbleUrl("https://music.example.com", "alice", "secret", "12", true, true);
  EXPECT_NE(std::string::npos, url.find("/rest/scrobble.view"));
  EXPECT_NE(std::string::npos, url.find("id=12"));
  EXPECT_NE(std::string::npos, url.find("submission=true"));
  EXPECT_NE(std::string::npos, url.find("p=enc:"));
}

TEST(ScrobblerEligibility, LastFmHalfOrFourMinutes) {
  Song song;
  song.set_valid(true);
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_length_nanosec(300LL * 1000000000LL);
  EXPECT_FALSE(ScrobblerEligibility::ShouldScrobble(song, 10LL * 1000000000LL));
  EXPECT_TRUE(ScrobblerEligibility::ShouldScrobble(song, 150LL * 1000000000LL));
  EXPECT_TRUE(ScrobblerEligibility::ShouldScrobble(song, 240LL * 1000000000LL));

  Song short_song;
  short_song.set_valid(true);
  short_song.set_title("Intro");
  short_song.set_artist("Band");
  short_song.set_length_nanosec(20LL * 1000000000LL);
  EXPECT_FALSE(ScrobblerEligibility::ShouldScrobble(short_song, 20LL * 1000000000LL));

  Song unknown;
  unknown.set_valid(true);
  unknown.set_title("Live");
  unknown.set_artist("Band");
  EXPECT_FALSE(ScrobblerEligibility::ShouldScrobble(unknown, 60LL * 1000000000LL));
  EXPECT_TRUE(ScrobblerEligibility::ShouldScrobble(unknown, 240LL * 1000000000LL));

  EXPECT_TRUE(ScrobblerEligibility::ShouldScrobble(song, 30LL * 1000000000LL, 10));
  EXPECT_FALSE(ScrobblerEligibility::ShouldScrobble(song, 20LL * 1000000000LL, 10));
}
