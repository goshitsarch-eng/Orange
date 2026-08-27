#include "core/song.h"
#include "scrobbler/scrobblepoint.h"
#include "scrobbler/scrobbletoggleicon.h"
#include "scrobbler/scrobblereligibility.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerlifecycle.h"
#include "scrobbler/scrobblerlovestate.h"
#include "scrobbler/scrobblerplayingstate.h"
#include "scrobbler/scrobblersubmittiming.h"
#include "scrobbler/lastfmscrobbler.h"
#include "scrobbler/listenbrainzscrobbler.h"
#include "scrobbler/listenbrainzscrobblestate.h"
#include "scrobbler/listenbrainzoauth.h"
#include "scrobbler/scrobblemetadata.h"
#include "scrobbler/scrobblercache.h"
#include "scrobbler/scrobblersources.h"
#include "scrobbler/subsonicscrobbler.h"
#include "scrobbler/subsonicscrobblestate.h"

#include <gtest/gtest.h>

TEST(ScrobblerLifecycle, FlushesOnExitAndSubmitsWhenLeavingOffline) {
  EXPECT_TRUE(ScrobblerLifecycle::ShouldFlushOnExit(true));
  EXPECT_FALSE(ScrobblerLifecycle::ShouldFlushOnExit(false));
  EXPECT_TRUE(ScrobblerLifecycle::ShouldSubmitAfterOfflineToggle(true, false));
  EXPECT_FALSE(ScrobblerLifecycle::ShouldSubmitAfterOfflineToggle(false, true));
  EXPECT_FALSE(ScrobblerLifecycle::ShouldSubmitAfterOfflineToggle(false, false));
  EXPECT_TRUE(ScrobblerLifecycle::ShouldSubmitOnStartup(true, false));
  EXPECT_FALSE(ScrobblerLifecycle::ShouldSubmitOnStartup(true, true));
  EXPECT_FALSE(ScrobblerLifecycle::ShouldSubmitOnStartup(false, false));
}

TEST(ScrobblerCache, RoundTripJson) {
  ScrobblerCacheItem item;
  item.timestamp = 1700000000;
  item.artist = "Portishead";
  item.album = "Dummy";
  item.title = "Roads";
  item.albumartist = "Portishead";
  item.track = 8;
  item.length_nanosec = 300000000000LL;
  item.musicbrainz_recording_id = "rec-1";
  item.musicbrainz_artist_id = "art-1/art-2";
  item.music_service = "tidal.com";
  item.share_url = "https://tidal.com/track/99";
  const std::string json = ScrobblerCache::ToJson({item});
  const auto parsed = ScrobblerCache::Parse(json);
  ASSERT_EQ(1u, parsed.size());
  EXPECT_EQ(1700000000u, parsed.front().timestamp);
  EXPECT_EQ("Portishead", parsed.front().artist);
  EXPECT_EQ("Dummy", parsed.front().album);
  EXPECT_EQ("Roads", parsed.front().title);
  EXPECT_EQ(8, parsed.front().track);
  EXPECT_EQ(300000000000LL, parsed.front().length_nanosec);
  EXPECT_EQ("rec-1", parsed.front().musicbrainz_recording_id);
  EXPECT_EQ("art-1/art-2", parsed.front().musicbrainz_artist_id);
  EXPECT_EQ("tidal.com", parsed.front().music_service);
  EXPECT_EQ("https://tidal.com/track/99", parsed.front().share_url);
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
  EXPECT_NE(std::string::npos, body.find("\"additional_info\""));
  EXPECT_NE(std::string::npos, body.find("\"media_player\":\"Strawberry\""));
  EXPECT_TRUE(ListenBrainzScrobbleState::ShouldIncludeReleaseName("High Voltage"));
  EXPECT_FALSE(ListenBrainzScrobbleState::ShouldIncludeReleaseName(""));
  EXPECT_EQ(5000, ListenBrainzScrobbleState::DurationMs(5000 * 1000000LL));
  EXPECT_EQ(0, ListenBrainzScrobbleState::DurationMs(0));
  const std::string extra = ListenBrainzScrobbleState::AdditionalInfoJson(3, 180000000000LL, "1.2.27");
  EXPECT_NE(std::string::npos, extra.find("\"duration_ms\":180000"));
  EXPECT_NE(std::string::npos, extra.find("\"tracknumber\":3"));
  EXPECT_NE(std::string::npos, extra.find("\"media_player_version\":\"1.2.27\""));
  ScrobblerCacheItem rich;
  rich.track = 2;
  rich.length_nanosec = 240000000000LL;
  rich.musicbrainz_recording_id = "rec-9";
  rich.musicbrainz_album_id = "rel-1";
  rich.musicbrainz_artist_id = "art-1/art-2";
  rich.musicbrainz_album_artist_id = "art-1";
  rich.musicbrainz_work_id = "work-1/work-2";
  rich.music_service = "spotify.com";
  rich.music_service_name = "Spotify";
  rich.share_url = "https://open.spotify.com/track/abc";
  rich.spotify_id = "abc";
  const std::string mbids = ListenBrainzScrobbleState::AdditionalInfoJson(rich, "1.2.27");
  EXPECT_NE(std::string::npos, mbids.find("\"recording_mbid\":\"rec-9\""));
  EXPECT_NE(std::string::npos, mbids.find("\"release_mbid\":\"rel-1\""));
  EXPECT_NE(std::string::npos, mbids.find("\"artist_mbids\":[\"art-1\",\"art-2\"]"));
  EXPECT_NE(std::string::npos, mbids.find("\"work_mbids\":[\"work-1\",\"work-2\"]"));
  EXPECT_NE(std::string::npos, mbids.find("\"music_service\":\"spotify.com\""));
  EXPECT_NE(std::string::npos, mbids.find("\"origin_url\":\"https://open.spotify.com/track/abc\""));
  EXPECT_NE(std::string::npos, mbids.find("\"spotify_id\":\"abc\""));
  item.musicbrainz_recording_id = "rec-9";
  item.music_service = "tidal.com";
  const std::string with_mbid = ListenBrainzScrobbler::SubmitBody("single", {item});
  EXPECT_NE(std::string::npos, with_mbid.find("\"recording_mbid\":\"rec-9\""));
  EXPECT_NE(std::string::npos, with_mbid.find("\"music_service\":\"tidal.com\""));
  EXPECT_STREQ("import", ListenBrainzScrobbleState::CachedListenType());
  EXPECT_EQ(5, ListenBrainzScrobbleState::DelaySeconds(0, false));
  EXPECT_EQ(30, ListenBrainzScrobbleState::DelaySeconds(0, true));
  EXPECT_EQ(10, ListenBrainzScrobbleState::DelaySeconds(10, false));
  EXPECT_EQ(45, ListenBrainzScrobbleState::DelaySeconds(45, true));
  EXPECT_TRUE(ListenBrainzScrobbleState::ShouldStartSubmitTimer(false, true, false));
  EXPECT_FALSE(ListenBrainzScrobbleState::ShouldStartSubmitTimer(true, true, false));
  EXPECT_FALSE(ListenBrainzScrobbleState::ShouldStartSubmitTimer(false, false, false));
  EXPECT_FALSE(ListenBrainzScrobbleState::ShouldStartSubmitTimer(false, true, true));
  const std::string imported = ListenBrainzScrobbler::SubmitBody(ListenBrainzScrobbleState::CachedListenType(), {item});
  EXPECT_NE(std::string::npos, imported.find("\"listen_type\":\"import\""));
}

TEST(SubsonicScrobbler, ScrobbleUrlUsesRestEndpoint) {
  const std::string url = SubsonicScrobbler::ScrobbleUrl("https://music.example.com", "alice", "secret", "12", true, true);
  EXPECT_NE(std::string::npos, url.find("/rest/scrobble.view"));
  EXPECT_NE(std::string::npos, url.find("id=12"));
  EXPECT_NE(std::string::npos, url.find("submission=true"));
  EXPECT_NE(std::string::npos, url.find("p=enc:"));
}

TEST(SubsonicScrobbleState, GatesNowPlayingAndSubmit) {
  Song subsonic(Song::Source::Subsonic);
  subsonic.set_url("subsonic://12");
  subsonic.set_song_id("12");
  EXPECT_TRUE(SubsonicScrobbleState::ShouldNowPlaying(subsonic, true));
  EXPECT_FALSE(SubsonicScrobbleState::ShouldNowPlaying(subsonic, false));
  Song local(Song::Source::Collection);
  local.set_url("file:///a");
  EXPECT_FALSE(SubsonicScrobbleState::ShouldNowPlaying(local, true));
  EXPECT_TRUE(SubsonicScrobbleState::ShouldSubmit(subsonic, "subsonic://12", "12"));
  EXPECT_FALSE(SubsonicScrobbleState::ShouldSubmit(subsonic, "subsonic://99", "99"));
  EXPECT_EQ("12", SubsonicScrobbleState::TrackId(subsonic));
  const std::string timed = SubsonicScrobbler::ScrobbleUrl("https://music.example.com", "alice", "secret", "12", false, true, 1700000000000);
  EXPECT_NE(std::string::npos, timed.find("time=1700000000000"));
  EXPECT_NE(std::string::npos, timed.find("submission=false"));
  EXPECT_TRUE(SubsonicScrobbleState::ShouldSubmitImmediately(0, false));
  EXPECT_FALSE(SubsonicScrobbleState::ShouldSubmitImmediately(10, false));
  EXPECT_FALSE(SubsonicScrobbleState::ShouldSubmitImmediately(0, true));
  EXPECT_TRUE(SubsonicScrobbleState::ShouldStartSubmitTimer(10, false, false));
  EXPECT_FALSE(SubsonicScrobbleState::ShouldStartSubmitTimer(10, false, true));
  EXPECT_FALSE(SubsonicScrobbleState::ShouldStartSubmitTimer(0, false, false));
  EXPECT_EQ(10, SubsonicScrobbleState::DelaySeconds(10));
  EXPECT_EQ(0, SubsonicScrobbleState::DelaySeconds(0));
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

TEST(ListenBrainzOAuth, AuthorizationUrlMatchesQtMusicBrainz) {
  EXPECT_STREQ("https://musicbrainz.org/oauth2/authorize", ListenBrainzOAuth::kAuthorizeUrl);
  EXPECT_STREQ("https://musicbrainz.org/oauth2/token", ListenBrainzOAuth::kTokenUrl);
  EXPECT_FALSE(ListenBrainzOAuth::ClientId().empty());
  EXPECT_FALSE(ListenBrainzOAuth::ClientSecret().empty());
  EXPECT_EQ("http://localhost:43111/", ListenBrainzOAuth::RedirectUri(43111));
  EXPECT_EQ("abc", ListenBrainzOAuth::ExtractCode("http://localhost:43111/?code=abc&state=x"));
  EXPECT_TRUE(ListenBrainzOAuth::ExtractCode("http://localhost:43111/").empty());
  EXPECT_FALSE(ListenBrainzOAuth::ShouldStartAuthorization(nullptr));
  EXPECT_TRUE(ListenBrainzOAuth::LoginWidgetSignedIn(true, false));
  EXPECT_TRUE(ListenBrainzOAuth::LoginWidgetSignedIn(false, true));
  EXPECT_FALSE(ListenBrainzOAuth::LoginWidgetSignedIn(false, false));
  const std::string url = ListenBrainzOAuth::AuthorizationUrl("http://localhost:43111/", "challenge");
  EXPECT_NE(std::string::npos, url.find("https://musicbrainz.org/oauth2/authorize"));
  EXPECT_NE(std::string::npos, url.find("response_type=code"));
  EXPECT_NE(std::string::npos, url.find("code_challenge=challenge"));
  EXPECT_NE(std::string::npos, url.find("scope="));
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
  song.set_musicbrainz_recording_id("rec-1");
  const auto original = ScrobbleMetadata::FromSong(song, 7);
  EXPECT_EQ("Beth Gibbons", original.artist);
  EXPECT_EQ("Roads (Deluxe Edition)", original.title);
  EXPECT_EQ("rec-1", original.musicbrainz_recording_id);
  EXPECT_TRUE(original.music_service.empty());
  const auto preferred = ScrobbleMetadata::FromSong(song, 7, true, true);
  EXPECT_EQ("Portishead", preferred.artist);
  EXPECT_EQ("Roads", preferred.title);
  EXPECT_EQ("Portishead", preferred.albumartist);
  Song tidal(Song::Source::Tidal);
  tidal.set_artist("A");
  tidal.set_title("T");
  tidal.set_song_id("99");
  const auto stream = ScrobbleMetadata::FromSong(tidal);
  EXPECT_EQ("tidal.com", stream.music_service);
  EXPECT_EQ("Tidal", stream.music_service_name);
  EXPECT_EQ("https://tidal.com/track/99", stream.share_url);
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

TEST(ScrobblePoint, ComputesHalfLengthClampedToLastFmWindow) {
  EXPECT_EQ(ScrobblePoint::kMaxNsecs, ScrobblePoint::Compute(0));
  EXPECT_EQ(ScrobblePoint::kMinNsecs, ScrobblePoint::Compute(40LL * ScrobblePoint::kNsecPerSec));
  EXPECT_EQ(90LL * ScrobblePoint::kNsecPerSec, ScrobblePoint::Compute(180LL * ScrobblePoint::kNsecPerSec));
  EXPECT_EQ(ScrobblePoint::kMaxNsecs, ScrobblePoint::Compute(600LL * ScrobblePoint::kNsecPerSec));
  EXPECT_EQ(100LL * ScrobblePoint::kNsecPerSec + ScrobblePoint::kMinNsecs, ScrobblePoint::Compute(40LL * ScrobblePoint::kNsecPerSec, 100LL * ScrobblePoint::kNsecPerSec));
  EXPECT_EQ(100LL * ScrobblePoint::kNsecPerSec + ScrobblePoint::kMaxNsecs, ScrobblePoint::Compute(0, 100LL * ScrobblePoint::kNsecPerSec));
  EXPECT_FALSE(ScrobblePoint::Reached(30LL * ScrobblePoint::kNsecPerSec, ScrobblePoint::kMinNsecs));
  EXPECT_TRUE(ScrobblePoint::Reached(31LL * ScrobblePoint::kNsecPerSec, ScrobblePoint::kMinNsecs));
  EXPECT_TRUE(ScrobblePoint::ShouldSubmit(true, false, true, ScrobblePoint::kMinNsecs, ScrobblePoint::kMinNsecs));
  EXPECT_FALSE(ScrobblePoint::ShouldSubmit(false, false, true, ScrobblePoint::kMinNsecs, ScrobblePoint::kMinNsecs));
  EXPECT_FALSE(ScrobblePoint::ShouldSubmit(true, true, true, ScrobblePoint::kMinNsecs, ScrobblePoint::kMinNsecs));
  EXPECT_FALSE(ScrobblePoint::ShouldSubmit(true, false, false, ScrobblePoint::kMinNsecs, ScrobblePoint::kMinNsecs));
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
