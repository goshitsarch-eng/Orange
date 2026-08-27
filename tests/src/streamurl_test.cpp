#include "core/song.h"
#include "core/taskmanager.h"
#include "core/urlhandlers.h"
#include "engine/enginediscoverer.h"
#include "dialogs/edittagsave.h"
#include "qobuz/qobuzmetadatarequest.h"
#include "streaming/streamingmetadatamerge.h"
#include "streaming/streamingmetadataqueue.h"
#include "qobuz/qobuzservice.h"
#include "qobuz/qobuzstreamurlrequest.h"
#include "qobuz/qobuzurlhandler.h"
#include "spotify/spotifymetadatarequest.h"
#include "spotify/spotifyplayback.h"
#include "spotify/spotifyservice.h"
#include "subsonic/subsonicservice.h"
#include "subsonic/subsonicurlhandler.h"
#include "tidal/tidalservice.h"
#include "tidal/tidalstreamurlrequest.h"
#include "tidal/tidalurlhandler.h"

#include <gtest/gtest.h>

TEST(TidalStreamUrlRequest, BuildsOriginalMethodUrls) {
  EXPECT_EQ("tracks/99/streamUrl", TidalStreamUrlRequest::Resource(TidalSettings::StreamUrlMethod::StreamUrl, "99"));
  EXPECT_EQ("tracks/99/urlpostpaywall", TidalStreamUrlRequest::Resource(TidalSettings::StreamUrlMethod::UrlPostPaywall, "99"));
  EXPECT_EQ("tracks/99/playbackinfopostpaywall",
            TidalStreamUrlRequest::Resource(TidalSettings::StreamUrlMethod::PlaybackInfoPostPaywall, "99"));
  EXPECT_EQ("https://api.tidalhifi.com/v1/tracks/99/streamUrl?soundQuality=LOSSLESS&countryCode=US",
            TidalStreamUrlRequest::Url("https://api.tidalhifi.com/v1", TidalSettings::StreamUrlMethod::StreamUrl, "99", "US", "LOSSLESS"));
  const std::string post =
      TidalStreamUrlRequest::Url("https://api.tidalhifi.com/v1", TidalSettings::StreamUrlMethod::UrlPostPaywall, "99", "NO", "HI_RES");
  EXPECT_NE(std::string::npos, post.find("/tracks/99/urlpostpaywall?"));
  EXPECT_NE(std::string::npos, post.find("audioquality=HI_RES"));
  EXPECT_NE(std::string::npos, post.find("playbackmode=STREAM"));
  EXPECT_NE(std::string::npos, post.find("assetpresentation=FULL"));
  EXPECT_NE(std::string::npos, post.find("urlusagemode=STREAM"));
  EXPECT_NE(std::string::npos, post.find("countryCode=NO"));
  const std::string playback = TidalStreamUrlRequest::Url("https://api.tidalhifi.com/v1",
                                                          TidalSettings::StreamUrlMethod::PlaybackInfoPostPaywall, "7", "US", "LOSSLESS");
  EXPECT_NE(std::string::npos, playback.find("/tracks/7/playbackinfopostpaywall?"));
  EXPECT_EQ(std::string::npos, playback.find("urlusagemode"));
  EXPECT_EQ(TidalSettings::StreamUrlMethod::UrlPostPaywall, TidalStreamUrlRequest::MethodFromSettings(1));
  EXPECT_EQ("99", TidalStreamUrlRequest::TrackId("tidal://99"));
  EXPECT_EQ("99", TidalStreamUrlRequest::TrackId("tidal:///99"));
}

TEST(TidalStreamUrlRequest, ParsesUrlCodecAndSample) {
  const UrlHandler::LoadResult result = TidalStreamUrlRequest::Parse(
      R"json({"trackId":99,"url":"https://cdn.example/t.flac","codec":"flac","sampleRate":44100,"bitDepth":16})json", "tidal://99",
      "99");
  EXPECT_EQ(UrlHandler::LoadResult::Type::TrackAvailable, result.type);
  EXPECT_EQ("https://cdn.example/t.flac", result.stream_url);
  EXPECT_EQ(Song::FileType::FLAC, result.filetype);
  EXPECT_EQ(44100, result.samplerate);
  EXPECT_EQ(16, result.bit_depth);
}

TEST(TidalStreamUrlRequest, ParsesUrlsArray) {
  const auto urls = TidalStreamUrlRequest::ParseUrls(R"json({"urls":["https://a","https://b"]})json");
  ASSERT_EQ(2u, urls.size());
  EXPECT_EQ("https://a", urls.front());
  const UrlHandler::LoadResult result =
      TidalStreamUrlRequest::Parse(R"json({"trackId":1,"urls":["https://a.mp3","https://b.mp3"]})json", "tidal://1", "1");
  EXPECT_EQ("https://a.mp3", result.stream_url);
  EXPECT_EQ(Song::FileType::MPEG, result.filetype);
}

TEST(TidalStreamUrlRequest, ParsesJsonManifestAndDash) {
  const std::string manifest = TidalStreamUrlRequest::EncodeBase64(
      R"json({"encryptionType":"NONE","urls":["https://cdn.example/a.flac"],"mimeType":"audio/flac"})json");
  const UrlHandler::LoadResult result =
      TidalStreamUrlRequest::Parse("{\"trackId\":8,\"manifest\":\"" + manifest + "\"}", "tidal://8", "8");
  EXPECT_EQ(UrlHandler::LoadResult::Type::TrackAvailable, result.type);
  EXPECT_EQ("https://cdn.example/a.flac", result.stream_url);
  EXPECT_EQ(Song::FileType::FLAC, result.filetype);

  const std::string dash = TidalStreamUrlRequest::EncodeBase64("<MPD></MPD>");
  const UrlHandler::LoadResult dash_result =
      TidalStreamUrlRequest::Parse("{\"trackId\":8,\"manifest\":\"" + dash + "\"}", "tidal://8", "8");
  EXPECT_EQ("data:application/dash+xml;base64," + dash, dash_result.stream_url);
}

TEST(TidalStreamUrlRequest, RejectsEncryptedStreams) {
  const std::string manifest = TidalStreamUrlRequest::EncodeBase64(R"json({"encryptionType":"OLD_AES","urls":["https://x"]})json");
  const UrlHandler::LoadResult encrypted =
      TidalStreamUrlRequest::Parse("{\"trackId\":1,\"manifest\":\"" + manifest + "\"}", "tidal://1", "1");
  EXPECT_EQ(UrlHandler::LoadResult::Type::Error, encrypted.type);
  EXPECT_NE(std::string::npos, encrypted.error.find("encrypted"));

  const UrlHandler::LoadResult keyed =
      TidalStreamUrlRequest::Parse(R"json({"trackId":1,"url":"https://x","encryptionKey":"abc"})json", "tidal://1", "1");
  EXPECT_EQ(UrlHandler::LoadResult::Type::Error, keyed.type);

  const UrlHandler::LoadResult missing = TidalStreamUrlRequest::Parse(R"json({"trackId":1})json", "tidal://1", "1");
  EXPECT_EQ("Missing stream urls.", missing.error);
}

TEST(QobuzStreamUrlRequest, SignsGetFileUrl) {
  const uint64_t timestamp = 1700000000;
  const std::string payload = QobuzStreamUrlRequest::SignaturePayload("99", 27, timestamp, "secret");
  EXPECT_EQ("trackgetFileUrlformat_id27intentstreamtrack_id991700000000secret", payload);
  EXPECT_EQ(QobuzStreamUrlRequest::Md5Hex(payload), QobuzStreamUrlRequest::Sign("99", 27, timestamp, "secret"));
  const std::string url =
      QobuzStreamUrlRequest::Url("https://www.qobuz.com/api.json/0.2", "99", 27, timestamp, "app", "secret", "tok");
  EXPECT_NE(std::string::npos, url.find("/track/getFileUrl?"));
  EXPECT_NE(std::string::npos, url.find("app_id=app"));
  EXPECT_NE(std::string::npos, url.find("format_id=27"));
  EXPECT_NE(std::string::npos, url.find("intent=stream"));
  EXPECT_NE(std::string::npos, url.find("track_id=99"));
  EXPECT_NE(std::string::npos, url.find("request_ts=1700000000"));
  EXPECT_NE(std::string::npos, url.find("request_sig=" + QobuzStreamUrlRequest::Sign("99", 27, timestamp, "secret")));
  EXPECT_NE(std::string::npos, url.find("user_auth_token=tok"));
}

TEST(QobuzStreamUrlRequest, ParsesFileUrl) {
  const UrlHandler::LoadResult result = QobuzStreamUrlRequest::Parse(
      R"json({"track_id":7,"url":"https://stream.qobuz/x","mime_type":"audio/flac","duration":180,"sampling_rate":44.1,"bit_depth":16})json",
      "qobuz://7", "7");
  EXPECT_EQ(UrlHandler::LoadResult::Type::TrackAvailable, result.type);
  EXPECT_EQ("https://stream.qobuz/x", result.stream_url);
  EXPECT_EQ(Song::FileType::FLAC, result.filetype);
  EXPECT_EQ(180000000000LL, result.duration);
  EXPECT_EQ(44000, result.samplerate);
  EXPECT_EQ(16, result.bit_depth);

  const UrlHandler::LoadResult wrong =
      QobuzStreamUrlRequest::Parse(R"json({"track_id":8,"url":"https://x","mime_type":"audio/flac"})json", "qobuz://7", "7");
  EXPECT_EQ("Incorrect track ID returned.", wrong.error);
}

TEST(QobuzMetadataRequest, ParsesTrackGet) {
  EXPECT_EQ("https://www.qobuz.com/api.json/0.2/track/get?track_id=7&app_id=app&user_auth_token=tok",
            QobuzMetadataRequest::Url("https://www.qobuz.com/api.json/0.2", "7", "app", "tok"));
  const Song song = QobuzMetadataRequest::ParseTrack(R"json({
    "id":7,
    "title":"Roads",
    "track_number":8,
    "media_number":1,
    "duration":300,
    "copyright":"(C)",
    "composer":{"name":"Barrow"},
    "performer":{"name":"Portishead"},
    "album":{
      "id":8,
      "title":"Dummy",
      "artist":{"id":1,"name":"Portishead"},
      "image":{"large":"https://i/l.jpg"},
      "genre":{"name":"Trip Hop"},
      "released_at":778377600
    }
  })json");
  EXPECT_TRUE(song.is_valid());
  EXPECT_EQ("Roads", song.title());
  EXPECT_EQ("Portishead", song.artist());
  EXPECT_EQ("Dummy", song.album());
  EXPECT_EQ("Trip Hop", song.genre());
  EXPECT_EQ("Barrow", song.composer());
  EXPECT_EQ("https://i/l.jpg", song.art_automatic());
  EXPECT_EQ(8, song.track());
  EXPECT_EQ(1, song.disc());
  EXPECT_EQ(300000000000LL, song.length_nanosec());
  EXPECT_EQ(1994, song.year());
}

TEST(SpotifyPlayback, UsesNativeUriWhenPluginAndTokenPresent) {
  EXPECT_TRUE(SpotifyPlayback::IsSpotifyUrl("spotify://abc"));
  EXPECT_TRUE(SpotifyPlayback::IsSpotifyUrl("spotify:track:abc"));
  EXPECT_FALSE(SpotifyPlayback::IsSpotifyUrl("https://open.spotify.com/track/abc"));
  EXPECT_EQ("abc", SpotifyPlayback::TrackId("spotify://abc"));
  EXPECT_EQ("abc", SpotifyPlayback::TrackId("spotify:track:abc"));
  EXPECT_EQ("spotify://abc", SpotifyPlayback::CanonicalPlayUrl("spotify:track:abc"));
  EXPECT_EQ("spotify://abc", SpotifyPlayback::CanonicalPlayUrl("spotify://abc"));
  EXPECT_TRUE(SpotifyPlayback::UseNativePlayback("spotify://abc", true, true));
  EXPECT_FALSE(SpotifyPlayback::UseNativePlayback("spotify://abc", false, true));
  EXPECT_FALSE(SpotifyPlayback::UseNativePlayback("spotify://abc", true, false));
  EXPECT_FALSE(SpotifyPlayback::UseNativePlayback("https://p.mp3", true, true));
  EXPECT_EQ("spotify://abc", SpotifyPlayback::EffectivePlayUrl("spotify://abc", "https://p.mp3", true, true));
  EXPECT_EQ("https://p.mp3", SpotifyPlayback::EffectivePlayUrl("spotify://abc", "https://p.mp3", false, true));
  const auto result = SpotifyPlayback::NativeResult("spotify:track:abc");
  EXPECT_EQ(UrlHandler::LoadResult::Type::TrackAvailable, result.type);
  EXPECT_EQ("spotify:track:abc", result.media_url);
  EXPECT_EQ("spotify://abc", result.stream_url);
  EXPECT_TRUE(SpotifyPlayback::ShouldSetAccessToken("tok", true));
  EXPECT_FALSE(SpotifyPlayback::ShouldSetAccessToken("", true));
  EXPECT_FALSE(SpotifyPlayback::ShouldSetAccessToken("tok", false));
  EXPECT_NE(std::string::npos, std::string(SpotifyPlayback::kOAuthScope).find("streaming"));
}

TEST(SpotifyMetadataRequest, ParsesTrackAndArtistGenre) {
  EXPECT_EQ("https://api.spotify.com/v1/tracks/abc", SpotifyMetadataRequest::TrackUrl("https://api.spotify.com/v1", "abc"));
  EXPECT_EQ("https://api.spotify.com/v1/artists/art", SpotifyMetadataRequest::ArtistUrl("https://api.spotify.com/v1", "art"));
  const Song song = SpotifyMetadataRequest::ParseTrack(R"json({
    "id":"abc",
    "name":"Roads",
    "uri":"spotify:track:abc",
    "track_number":8,
    "disc_number":1,
    "duration_ms":300000,
    "preview_url":"https://p.mp3",
    "artists":[{"id":"art","name":"Portishead"}],
    "album":{
      "id":"alb",
      "name":"Dummy",
      "artists":[{"name":"Portishead"}],
      "release_date":"1994-08-29",
      "images":[{"url":"https://i/big.jpg","width":640,"height":640}]
    }
  })json");
  EXPECT_TRUE(song.is_valid());
  EXPECT_EQ("Roads", song.title());
  EXPECT_EQ("Portishead", song.artist());
  EXPECT_EQ("art", song.artist_id());
  EXPECT_EQ("Dummy", song.album());
  EXPECT_EQ("https://p.mp3", song.stream_url());
  EXPECT_EQ("https://i/big.jpg", song.art_automatic());
  EXPECT_EQ(1994, song.year());
  EXPECT_EQ(300000000000LL, song.length_nanosec());
  EXPECT_EQ("trip hop", SpotifyMetadataRequest::ParseArtistGenre(R"json({"genres":["trip hop","electronic"]})json"));
}

TEST(StreamingMetadataQueue, TrackIdsAndMergeMatchQt) {
  EXPECT_EQ(200, StreamingMetadataQueue::kDelayMs);
  EXPECT_TRUE(StreamingMetadataQueue::ShouldStart(false, false));
  EXPECT_FALSE(StreamingMetadataQueue::ShouldStart(true, false));
  EXPECT_FALSE(StreamingMetadataQueue::ShouldStart(false, true));
  EXPECT_TRUE(StreamingMetadataQueue::ShouldContinue(false));
  EXPECT_FALSE(StreamingMetadataQueue::ShouldContinue(true));

  Song qobuz(Song::Source::Qobuz);
  qobuz.set_song_id("7");
  qobuz.set_url("qobuz://ignored");
  EXPECT_EQ("7", StreamingMetadataQueue::TrackId(qobuz));
  EXPECT_EQ("99", StreamingMetadataQueue::QobuzTrackId({}, "qobuz://99"));
  EXPECT_EQ("99", StreamingMetadataQueue::QobuzTrackId({}, "qobuz:///99"));
  EXPECT_TRUE(StreamingMetadataQueue::ShouldEnqueue(qobuz));

  Song spotify(Song::Source::Spotify);
  spotify.set_url("spotify:track:abc");
  EXPECT_EQ("abc", StreamingMetadataQueue::TrackId(spotify));
  EXPECT_EQ("xyz", StreamingMetadataQueue::SpotifyTrackId("xyz", "spotify:track:abc"));
  EXPECT_EQ("abc", StreamingMetadataQueue::SpotifyTrackId({}, "spotify://abc"));
  EXPECT_TRUE(StreamingMetadataQueue::ShouldEnqueue(spotify));

  Song tidal(Song::Source::Tidal);
  tidal.set_song_id("1");
  tidal.set_url("tidal://1");
  EXPECT_TRUE(StreamingMetadataQueue::TrackId(tidal).empty());
  EXPECT_FALSE(StreamingMetadataQueue::ShouldEnqueue(tidal));
  EXPECT_FALSE(StreamingMetadataQueue::ShouldEnqueue(Song::Source::Qobuz, {}));

  SongList songs = {qobuz, tidal, spotify};
  const auto queued = StreamingMetadataQueue::EntriesFromSelection(songs, {2, 4, 6});
  ASSERT_EQ(2u, queued.size());
  EXPECT_EQ(Song::Source::Qobuz, queued[0].source);
  EXPECT_EQ("7", queued[0].track_id);
  EXPECT_EQ(2, queued[0].row);
  EXPECT_EQ(Song::Source::Spotify, queued[1].source);
  EXPECT_EQ("abc", queued[1].track_id);
  EXPECT_EQ(6, queued[1].row);

  Song original(Song::Source::Qobuz);
  original.set_valid(true);
  original.set_url("qobuz://7");
  original.set_title("Old");
  original.set_artist("Keep");
  original.set_track(3);
  original.set_year(-1);
  Song fetched(Song::Source::Qobuz);
  fetched.set_valid(true);
  fetched.set_title("Roads");
  fetched.set_album("Dummy");
  fetched.set_albumartist("Portishead");
  fetched.set_composer("Barrow");
  fetched.set_performer("Portishead");
  fetched.set_comment("(C)");
  fetched.set_genre("Trip Hop");
  fetched.set_disc(1);
  fetched.set_year(1994);
  fetched.set_length_nanosec(300000000000LL);
  fetched.set_art_automatic("https://i/l.jpg");
  StreamingMetadataMerge::Apply(&original, fetched);
  EXPECT_EQ("Roads", original.title());
  EXPECT_EQ("Keep", original.artist());
  EXPECT_EQ("Dummy", original.album());
  EXPECT_EQ("Portishead", original.albumartist());
  EXPECT_EQ("Barrow", original.composer());
  EXPECT_EQ("Portishead", original.performer());
  EXPECT_EQ("(C)", original.comment());
  EXPECT_EQ("Trip Hop", original.genre());
  EXPECT_EQ(3, original.track());
  EXPECT_EQ(1, original.disc());
  EXPECT_EQ(1994, original.year());
  EXPECT_EQ(300000000000LL, original.length_nanosec());
  EXPECT_EQ("https://i/l.jpg", original.art_automatic());
  EXPECT_FALSE(StreamingMetadataMerge::ShouldApply(Song()));
  EXPECT_EQ(0, EditTagSave::ResolveRow({original}, 2, "qobuz://7"));
}

TEST(SubsonicUrlHandler, BuildsStreamUrlFromPath) {
  EXPECT_EQ("99", SubsonicUrlHandler::SongId("subsonic://99"));
  const std::string url = SubsonicUrlHandler::StreamUrl("https://music.example.com", "alice", "secret", "99", true);
  EXPECT_NE(std::string::npos, url.find("/rest/stream.view"));
  EXPECT_NE(std::string::npos, url.find("id=99"));
  EXPECT_NE(std::string::npos, url.find("p=enc:"));
}

TEST(TidalService, AuthorizationUrlReceivedEmptyCodeFails) {
  TidalService tidal(nullptr);
  std::string error;
  tidal.AuthenticationFailed.Connect([&error](const std::string &text) { error = text; });
  tidal.AuthorizationUrlReceived("tidal://login/auth");
  EXPECT_EQ("No authorization code", error);
  error.clear();
  tidal.AuthorizationUrlReceived("tidal://login/auth?code=abc");
  EXPECT_EQ("Request URL is missing state!", error);
  error.clear();
  tidal.RememberPkce("verifier", "challenge");
  tidal.AuthorizationUrlReceived("tidal://login/auth?code=abc&state=wrong");
  EXPECT_EQ("Request URL has wrong state wrong != challenge", error);
  error.clear();
  tidal.AuthorizationUrlReceived("tidal://login/auth?code=abc&state=challenge");
  EXPECT_EQ("No network", error);
}

TEST(StreamingUrlHandlers, SchemesMatchServices) {
  TidalService tidal(nullptr);
  TidalUrlHandler tidal_handler(&tidal);
  EXPECT_EQ("tidal", tidal_handler.scheme());
  QobuzService qobuz(nullptr);
  QobuzUrlHandler qobuz_handler(&qobuz);
  EXPECT_EQ("qobuz", qobuz_handler.scheme());
  SubsonicService subsonic(nullptr);
  SubsonicUrlHandler subsonic_handler(&subsonic);
  EXPECT_EQ("subsonic", subsonic_handler.scheme());
  TaskManager tasks;
  TidalUrlHandler tasked(&tidal, &tasks);
  const UrlHandler::LoadResult result = tasked.Load("tidal://1");
  EXPECT_EQ(UrlHandler::LoadResult::Type::Error, result.type);
  EXPECT_TRUE(tasks.GetTasks().empty());
}

TEST(Song, FiletypeByMimeType) {
  EXPECT_EQ(Song::FileType::FLAC, Song::FiletypeByMimeType("audio/flac"));
  EXPECT_EQ(Song::FileType::MPEG, Song::FiletypeByMimeType("audio/mpeg"));
  EXPECT_EQ(Song::FileType::MP4, Song::FiletypeByMimeType("audio/mp4"));
  EXPECT_EQ(Song::FileType::OggVorbis, Song::FiletypeByMimeType("audio/ogg"));
  EXPECT_EQ(Song::FileType::OggOpus, Song::FiletypeByMimeType("audio/x-opus"));
  EXPECT_EQ(Song::FileType::ALAC, Song::FiletypeByMimeType("audio/x-alac"));
  EXPECT_EQ(Song::FileType::APE, Song::FiletypeByMimeType("audio/x-ffmpeg-parsed-ape"));
}

TEST(Song, FiletypeByDescription) {
  EXPECT_EQ(Song::FileType::FLAC, Song::FiletypeByDescription("Free Lossless Audio Codec (FLAC)"));
  EXPECT_EQ(Song::FileType::MPEG, Song::FiletypeByDescription("MPEG-1 Layer 3 (MP3)"));
  EXPECT_EQ(Song::FileType::MP4, Song::FiletypeByDescription("MPEG-4 AAC"));
  EXPECT_EQ(Song::FileType::OggVorbis, Song::FiletypeByDescription("Vorbis"));
  EXPECT_EQ(Song::FileType::ALAC, Song::FiletypeByDescription("Apple Lossless Audio Codec (ALAC)"));
  EXPECT_EQ(Song::FileType::Unknown, Song::FiletypeByDescription("not a codec"));
}

TEST(EngineDiscoverer, SkipsEmptyAndSpotifyUrls) {
  EXPECT_EQ(10, EngineDiscoverer::kDiscoveryTimeoutS);
  EXPECT_FALSE(EngineDiscoverer::ShouldDiscover(""));
  EXPECT_FALSE(EngineDiscoverer::ShouldDiscover("spotify:track:abc"));
  EXPECT_FALSE(EngineDiscoverer::ShouldDiscover("spotify://track/abc"));
  EXPECT_TRUE(EngineDiscoverer::ShouldDiscover("https://cdn.example/live.mp3"));
  EXPECT_TRUE(EngineDiscoverer::ShouldDiscover("tidal://1"));
  EXPECT_TRUE(EngineDiscoverer::ShouldDiscover("file:///tmp/a.flac"));
  EXPECT_EQ("https://cdn.example/a.flac", EngineDiscoverer::PlayUrl("tidal://1", "https://cdn.example/a.flac"));
  EXPECT_EQ("file:///tmp/a.flac", EngineDiscoverer::PlayUrl("file:///tmp/a.flac", {}));
}

TEST(EngineDiscoverer, MapsAudioInfoAndFiletypeLikeQt) {
  const EngineMetadata meta = EngineDiscoverer::FromAudioInfo(48000, 24, 320000);
  EXPECT_EQ(48000, meta.samplerate);
  EXPECT_EQ(24, meta.bitdepth);
  EXPECT_EQ(320, meta.bitrate);
  EXPECT_EQ(Song::FileType::Unknown, EngineDiscoverer::FiletypeFromCapsMimetype("audio/mpeg"));
  EXPECT_EQ(Song::FileType::Unknown, EngineDiscoverer::FiletypeFromCapsMimetype(""));
  EXPECT_EQ(Song::FileType::FLAC, EngineDiscoverer::FiletypeFromCapsMimetype("audio/x-flac"));
  EXPECT_EQ(Song::FileType::MPEG, EngineDiscoverer::ResolveFiletype("audio/mpeg", "MPEG-1 Layer 3 (MP3)"));
  EXPECT_EQ(Song::FileType::MP4, EngineDiscoverer::ResolveFiletype("audio/mpeg", "MPEG-4 AAC"));
  EXPECT_EQ(Song::FileType::OggVorbis, EngineDiscoverer::ResolveFiletype("audio/x-vorbis", "Vorbis"));
  EXPECT_EQ(EngineMetadata::Type::Current, EngineDiscoverer::MatchType("https://cdn.example/a", "https://cdn.example/a", "https://cdn.example/b"));
  EXPECT_EQ(EngineMetadata::Type::Next, EngineDiscoverer::MatchType("https://cdn.example/b", "https://cdn.example/a", "https://cdn.example/b"));
  EXPECT_EQ(EngineMetadata::Type::Any, EngineDiscoverer::MatchType("https://cdn.example/c", "https://cdn.example/a", "https://cdn.example/b"));
  EXPECT_STREQ("The discovery timed-out", EngineDiscoverer::ErrorMessage(EngineDiscoverer::kResultTimeout));
  EXPECT_STREQ("The URI is invalid", EngineDiscoverer::ErrorMessage(EngineDiscoverer::kResultUriInvalid));
}
