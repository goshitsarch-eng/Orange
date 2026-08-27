#include "core/song.h"
#include "scrobbler/scrobbletoggleicon.h"
#include "scrobbler/scrobblereligibility.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerlovestate.h"
#include "scrobbler/scrobblerplayingstate.h"
#include "scrobbler/scrobblersubmittiming.h"
#include "scrobbler/lastfmscrobbler.h"
#include "scrobbler/listenbrainzscrobbler.h"
#include "scrobbler/scrobblemetadata.h"
#include "scrobbler/scrobblercache.h"
#include "scrobbler/scrobblersources.h"
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

TEST(ScrobblerPlayingState, SameSongTimestampAndRadioPrev) {
  Song playing;
  playing.set_id(7);
  playing.set_url("http://radio.example/live");
  playing.set_artist("DJ");
  playing.set_title("Mix");
  playing.set_source(Song::Source::SomaFM);
  Song same = playing;
  EXPECT_TRUE(ScrobblerPlayingState::SameAsPlaying(same, playing));
  same.set_url("http://other");
  EXPECT_FALSE(ScrobblerPlayingState::SameAsPlaying(same, playing));
  EXPECT_EQ(40, ScrobblerPlayingState::ElapsedSeconds(100, 140));
  EXPECT_EQ(0, ScrobblerPlayingState::ElapsedSeconds(0, 140));
  EXPECT_TRUE(ScrobblerPlayingState::ShouldScrobbleRadioPrev(playing, false, 40));
  EXPECT_FALSE(ScrobblerPlayingState::ShouldScrobbleRadioPrev(playing, true, 40));
  EXPECT_FALSE(ScrobblerPlayingState::ShouldScrobbleRadioPrev(playing, false, 30));
  Song local;
  local.set_url("file:///a.flac");
  local.set_artist("A");
  local.set_title("B");
  local.set_source(Song::Source::LocalFile);
  EXPECT_FALSE(ScrobblerPlayingState::ShouldScrobbleRadioPrev(local, false, 90));
  EXPECT_EQ(12u, ScrobblerPlayingState::TimestampOrNow(12, 99));
  EXPECT_EQ(99u, ScrobblerPlayingState::TimestampOrNow(0, 99));
}

TEST(ScrobblerSources, EmptyAllowsEverySource) {
  EXPECT_TRUE(ScrobblerSources::Parse("").empty());
  EXPECT_TRUE(ScrobblerSources::Allows("", Song::Source::Tidal));
  EXPECT_TRUE(ScrobblerSources::Allows("  ", Song::Source::Collection));
}

TEST(ScrobblerSources, ParseJoinAndAllows) {
  const std::vector<int> parsed = ScrobblerSources::Parse("2, 6,11");
  ASSERT_EQ(3u, parsed.size());
  EXPECT_EQ(2, parsed[0]);
  EXPECT_EQ(6, parsed[1]);
  EXPECT_EQ(11, parsed[2]);
  EXPECT_EQ("2,6,11", ScrobblerSources::Join(parsed));
  EXPECT_TRUE(ScrobblerSources::Allows("2,6", Song::Source::Collection));
  EXPECT_TRUE(ScrobblerSources::Allows("2,6", Song::Source::Tidal));
  EXPECT_FALSE(ScrobblerSources::Allows("2,6", Song::Source::Spotify));
  EXPECT_FALSE(ScrobblerSources::Allows("1", Song::Source::Unknown));
}

TEST(LastFmScrobbler, AuthorizationUrlIncludesKeyAndToken) {
  const std::string url = LastFmScrobbler::AuthorizationUrl("abc123");
  EXPECT_EQ(std::string(LastFmScrobbler::kAuthUrl) + "?api_key=" + LastFmScrobbler::kApiKey + "&token=abc123", url);
  EXPECT_EQ(std::string(LastFmScrobbler::kAuthUrl) + "?api_key=" + LastFmScrobbler::kApiKey, LastFmScrobbler::AuthorizationUrl(""));
}

TEST(ListenBrainzScrobbler, LoveBodyEscapesMbid) {
  EXPECT_EQ("{\"recording_mbid\":\"mb\\\"id\",\"score\":1}", ListenBrainzScrobbler::LoveBody("mb\"id"));
}

TEST(ScrobbleMetadata, StripRemasteredAndAlbumArtist) {
  EXPECT_EQ("Roads", ScrobbleMetadata::StripRemasteredTitle("Roads (Remastered)"));
  EXPECT_EQ("Roads", ScrobbleMetadata::StripRemasteredTitle("Roads [2012 Remaster]"));
  EXPECT_EQ("Roads (Live)", ScrobbleMetadata::StripRemasteredTitle("Roads (Live)"));
  Song song;
  song.set_artist("Beth Gibbons");
  song.set_albumartist("Portishead");
  song.set_title("Roads (Deluxe Edition)");
  song.set_album("Dummy");
  const auto original = ScrobbleMetadata::FromSong(song, 7);
  EXPECT_EQ("Beth Gibbons", original.artist);
  EXPECT_EQ("Roads (Deluxe Edition)", original.title);
  const auto preferred = ScrobbleMetadata::FromSong(song, 7, true, true);
  EXPECT_EQ("Portishead", preferred.artist);
  EXPECT_EQ("Roads", preferred.title);
  EXPECT_EQ("Portishead", preferred.albumartist);
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

TEST(ScrobblerSubmitTiming, UsesConfiguredDelayWithErrorFloor) {
  EXPECT_EQ(0, ScrobblerSubmitTiming::DelaySeconds(0, false));
  EXPECT_EQ(0, ScrobblerSubmitTiming::DelaySeconds(-1, true));
  EXPECT_EQ(10, ScrobblerSubmitTiming::DelaySeconds(10, false));
  EXPECT_EQ(30, ScrobblerSubmitTiming::DelaySeconds(10, true));
  EXPECT_EQ(5, ScrobblerSubmitTiming::DelaySeconds(3, false));
  EXPECT_EQ(45, ScrobblerSubmitTiming::DelaySeconds(45, true));
}

TEST(ScrobblerLoveState, GatesOnEnabledMetadataAndLoved) {
  Song song;
  song.set_url("file:///tmp/roads.flac");
  song.set_artist("Portishead");
  song.set_title("Roads");
  EXPECT_TRUE(ScrobblerLoveState::CanLove(true, song));
  EXPECT_FALSE(ScrobblerLoveState::CanLove(false, song));
  Song incomplete;
  incomplete.set_url("file:///tmp/x.flac");
  EXPECT_FALSE(ScrobblerLoveState::CanLove(true, incomplete));
  EXPECT_TRUE(ScrobblerLoveState::DisableAfterLove());
  EXPECT_TRUE(ScrobblerLoveState::ResetLovedOnSongChange("file:///a", "file:///b"));
  EXPECT_FALSE(ScrobblerLoveState::ResetLovedOnSongChange("file:///a", "file:///a"));
}

TEST(ScrobblerError, DialogAndMessages) {
  EXPECT_TRUE(ScrobblerError::ShouldShowDialog(true, "Last.fm: request failed"));
  EXPECT_FALSE(ScrobblerError::ShouldShowDialog(true, {}));
  EXPECT_FALSE(ScrobblerError::ShouldShowDialog(false, "Last.fm: request failed"));
  EXPECT_EQ("Last.fm: request failed", ScrobblerError::RequestFailed("Last.fm"));
  EXPECT_EQ("ListenBrainz: not authenticated", ScrobblerError::NotAuthenticated("ListenBrainz"));
  EXPECT_STREQ("document-send-symbolic", ScrobbleToggleIcon::Name(true));
  EXPECT_STREQ("mail-send-symbolic", ScrobbleToggleIcon::Name(false));
}
