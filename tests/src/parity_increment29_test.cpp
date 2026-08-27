#include "core/filesystemwatcherinotify.h"
#include "core/httpbaserequest.h"
#include "core/jsonbaserequest.h"
#include "core/networkproxyfactory.h"
#include "core/networktimeoutpolicy.h"
#include "core/networktimeouts.h"
#include "core/settings.h"
#include "covermanager/coverutils.h"
#include "engine/enginebase.h"
#include "engine/gstengineproxy.h"
#include "engine/gstbusmessageevent.h"
#include "engine/gstfastspectrum.h"
#include "filterparser/filterparserintgecomparator.h"
#include "filterparser/filterparsersearchcomparators.h"
#include "filterparser/filtertreeand.h"
#include "filterparser/filtertreecolumnterm.h"
#include "filterparser/filtertreenop.h"
#include "filterparser/filtertreenot.h"
#include "filterparser/filtertreeor.h"
#include "filterparser/filtertreeterm.h"
#include "globalshortcuts/globalshortcut.h"
#include "lyrics/lyricssearchrequest.h"
#include "organize/organizeformat.h"
#include "organize/organizeformatvalidator.h"
#include "playlist/playlistundocommandinsertitems.h"
#include "scrobbler/scrobblemetadata.h"
#include "tagfetcher/acoustidclient.h"
#include "tagfetcher/musicbrainzclient.h"
#include "translations/translations.h"
#include "constants/notificationssettings.h"
#include "utilities/colorutils.h"
#include "utilities/cryptutils.h"
#include "utilities/envutils.h"
#include "utilities/mimeutils.h"
#include "utilities/randutils.h"
#include "utilities/xmlutils.h"

#include <glib.h>

#include <gtest/gtest.h>

#include <memory>

TEST(FilterTree, AndOrNotAccept) {
  Song foxes;
  foxes.set_title("Helplessness Blues");
  foxes.set_artist("Fleet Foxes");
  foxes.set_genre("Folk");
  foxes.set_year(2011);

  auto term = std::make_unique<FilterTreeTerm>(std::make_unique<FilterParserTextContainsComparator>("fleet"));
  EXPECT_TRUE(term->accept(foxes));

  auto column = std::make_unique<FilterTreeColumnTerm>(FilterColumn::Year, std::make_unique<FilterParserIntGeComparator>(2010));
  EXPECT_TRUE(column->accept(foxes));

  auto and_tree = std::make_unique<FilterTreeAnd>();
  and_tree->Add(std::make_unique<FilterTreeTerm>(std::make_unique<FilterParserTextContainsComparator>("folk")));
  and_tree->Add(std::make_unique<FilterTreeColumnTerm>(FilterColumn::Year, std::make_unique<FilterParserIntGeComparator>(2010)));
  EXPECT_TRUE(and_tree->accept(foxes));

  auto not_tree = std::make_unique<FilterTreeNot>(
      std::make_unique<FilterTreeTerm>(std::make_unique<FilterParserTextContainsComparator>("radiohead")));
  EXPECT_TRUE(not_tree->accept(foxes));
  EXPECT_EQ(FilterTreeNop().type(), FilterTree::FilterType::Nop);
}

TEST(AcoustidClient, ParsesRecordingMbids) {
  const std::string json = R"json({"results":[{"recordings":[{"id":"mbid-one"},{"id":"mbid-two"}]}]})json";
  const auto mbids = AcoustidClient::ParseMbids(json);
  ASSERT_EQ(2u, mbids.size());
  EXPECT_EQ("mbid-one", mbids[0]);
  EXPECT_EQ("mbid-two", mbids[1]);
}

TEST(MusicBrainzClient, ParsesRecordings) {
  const std::string json = R"json({
    "recordings": [
      {
        "id": "rec-1",
        "title": "Ragged Wood",
        "artist-credit": [{"name": "Fleet Foxes"}],
        "releases": [{"title": "Fleet Foxes", "id": "rel-1", "date": "2008-06-03"}]
      }
    ]
  })json";
  const auto results = MusicBrainzClient::ParseResults(json);
  ASSERT_FALSE(results.empty());
  EXPECT_EQ("Ragged Wood", results[0].title);
  EXPECT_EQ("Fleet Foxes", results[0].artist);
  EXPECT_EQ(2008, results[0].year);
  EXPECT_EQ("rec-1", results[0].musicbrainz_recording_id);
  EXPECT_EQ("rel-1", results[0].musicbrainz_album_id);
  const SongList songs = MusicBrainzClient::ToSongs(results);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("rec-1", songs.front().musicbrainz_recording_id());
  EXPECT_EQ("rel-1", songs.front().musicbrainz_album_id());
}

TEST(OrganizeFormat, ValidatesBracesAndTokens) {
  EXPECT_TRUE(OrganizeFormat("%artist/%title").IsValid());
  EXPECT_FALSE(OrganizeFormat("%artist/{%title").IsValid());
  std::string error;
  EXPECT_FALSE(OrganizeFormatValidator::IsValid("", &error));
  EXPECT_EQ("Format is empty", error);
}

TEST(NetworkHelpers, ProxyAndParams) {
  Settings s;
  s.BeginGroup("NetworkProxy");
  s.SetValue("type", "manual");
  s.SetValue("hostname", "127.0.0.1");
  s.SetIntValue("port", 8080);
  s.SetBoolValue("use_authentication", false);
  s.SetValue("username", "");
  s.SetValue("password", "");
  s.Sync();
  NetworkProxyFactory factory;
  factory.ReloadSettings();
  EXPECT_EQ(NetworkProxyFactory::Mode::Manual, factory.mode());
  EXPECT_EQ("http://127.0.0.1:8080", factory.ProxyUri());
  EXPECT_EQ("http", factory.Scheme());
  s.SetIntValue("mode", static_cast<int>(NetworkProxySettings::Mode::Direct));
  s.Sync();
  factory.ReloadSettings();
  EXPECT_EQ(NetworkProxyFactory::Mode::Direct, factory.mode());
  EXPECT_TRUE(factory.ProxyUri().empty());
  s.SetIntValue("mode", static_cast<int>(NetworkProxySettings::Mode::Manual));
  s.SetIntValue("type", static_cast<int>(NetworkProxySettings::ProxyType::Socks5Proxy));
  s.SetBoolValue("use_authentication", true);
  s.SetValue("username", "user");
  s.SetValue("password", "secret");
  s.Sync();
  factory.ReloadSettings();
  EXPECT_EQ("socks5://user:secret@127.0.0.1:8080", factory.ProxyUri());
  EXPECT_TRUE(factory.EngineOptions().address.empty());
  const std::string encoded = HttpBaseRequest::EncodeParams({{"q", "fleet foxes"}, {"fmt", "json"}});
  EXPECT_NE(std::string::npos, encoded.find("q="));
  EXPECT_NE(std::string::npos, encoded.find("fmt=json"));
  EXPECT_TRUE(JsonBaseRequest::IsObject("{\"ok\":true}"));
  EXPECT_FALSE(JsonBaseRequest::IsObject("[1]"));
  NetworkTimeouts timeouts;
  timeouts.SetTimeout(10);
  EXPECT_EQ(10, timeouts.timeout());
  EXPECT_EQ(5000, NetworkTimeoutPolicy::kAcoustidTimeoutMs);
  EXPECT_EQ(8000, NetworkTimeoutPolicy::kMusicBrainzTimeoutMs);
  EXPECT_EQ(6000, NetworkTimeoutPolicy::kCoverImageTimeoutMs);
  EXPECT_EQ(30000, NetworkTimeoutPolicy::kSubsonicTimeoutMs);
  EXPECT_EQ("Request timed out", NetworkTimeoutPolicy::TimedOutMessage());
  EXPECT_TRUE(NetworkTimeoutPolicy::IsCancelled("Operation was cancelled"));
  EXPECT_TRUE(NetworkTimeoutPolicy::IsCancelled("Request timed out"));
  EXPECT_FALSE(NetworkTimeoutPolicy::IsCancelled("connection refused"));
  EXPECT_EQ("Request timed out", NetworkTimeoutPolicy::FailureMessage("Cancelled", "fallback"));
  EXPECT_EQ("fallback", NetworkTimeoutPolicy::FailureMessage({}, "fallback"));
}

TEST(UtilitiesParity, ColorCryptXmlRandEnvMime) {
  EXPECT_NE(std::string::npos, ColorUtils::ColorToRgba(10, 20, 30, 0.5).find("rgba"));
  EXPECT_EQ("#6696e3", ColorUtils::HexToCss(OSDPrettySettings::kPresetBlue));
  EXPECT_EQ("#ca1610", ColorUtils::HexToCss(OSDPrettySettings::kPresetRed));
  EXPECT_EQ(OSDPrettySettings::kPresetBlue, ColorUtils::ParseHex(ColorUtils::HexToCss(OSDPrettySettings::kPresetBlue)));
  EXPECT_TRUE(ColorUtils::IsColorDark(0, 0, 0));
  EXPECT_FALSE(ColorUtils::IsColorDark(255, 255, 255));
  const std::string digest = CryptUtils::HexEncode(CryptUtils::HmacSha1("key", "data"));
  EXPECT_EQ(40u, digest.size());
  EXPECT_EQ("&lt;hi&gt;", XmlUtils::Escape("<hi>"));
  EXPECT_EQ("<hi>", XmlUtils::Unescape("&lt;hi&gt;"));
  EXPECT_EQ(8u, RandUtils::GetRandomStringWithChars(8).size());
  EnvUtils::Set("STRAWBERRY_TEST_ENV", "yes");
  EXPECT_TRUE(EnvUtils::Has("STRAWBERRY_TEST_ENV"));
  EXPECT_EQ("yes", EnvUtils::Get("STRAWBERRY_TEST_ENV"));
  EXPECT_EQ("audio/mpeg", MimeUtils::MimeTypeFromPath("song.mp3"));
  EXPECT_FALSE(Translations::Tr("Play").empty());
}

TEST(ScrobbleAndCover, MetadataAndImageExt) {
  Song song;
  song.set_artist("A");
  song.set_title("T");
  song.set_album("L");
  const auto meta = ScrobbleMetadata::FromSong(song, 42);
  EXPECT_EQ("A", meta.artist);
  EXPECT_EQ(42u, meta.timestamp);
  const std::string jpeg("\xff\xd8\xff", 3);
  EXPECT_EQ("jpg", CoverUtils::ExtensionForData(jpeg));
}

TEST(EngineHelpers, SpectrumAndBusEvent) {
  GstFastSpectrum spectrum;
  spectrum.set_bands(8);
  const float samples[8] = {0.1f, 0.2f, 0.0f, -0.1f, 0.3f, 0.0f, 0.4f, -0.2f};
  spectrum.Process(samples, 8);
  EXPECT_EQ(8u, spectrum.magnitudes().size());
  GstBusMessageEvent event(nullptr, 7);
  EXPECT_EQ(7u, event.generation());
  EXPECT_EQ(nullptr, event.message());
}

TEST(FileSystemWatcherInotify, AddAndClear) {
  FileSystemWatcherInotify watcher;
  watcher.AddPath("/tmp");
  watcher.RemovePath("/tmp");
  watcher.Clear();
}

TEST(GlobalShortcut, StoresIdAndKey) {
  GlobalShortcut shortcut("playpause", "Play/Pause", "MediaPlay");
  EXPECT_EQ("playpause", shortcut.id());
  EXPECT_EQ("MediaPlay", shortcut.default_key());
  shortcut.set_key("space");
  EXPECT_EQ("space", shortcut.key());
}

TEST(PlaylistUndo, InsertCommandKeepsSongs) {
  Song song;
  song.set_title("Demo");
  PlaylistUndoCommandInsertItems command(2, {song});
  EXPECT_EQ(PlaylistUndoCommandBase::Type::InsertItems, command.type());
  EXPECT_EQ(2, command.row());
  ASSERT_EQ(1u, command.songs().size());
  EXPECT_EQ("Demo", command.songs()[0].title());
}

TEST(LyricsSearchRequest, HoldsFields) {
  LyricsSearchRequest request;
  request.artist = "Fleet Foxes";
  request.title = "Ragged Wood";
  request.duration = 300000000;
  EXPECT_EQ("Fleet Foxes", request.artist);
  EXPECT_EQ("Ragged Wood", request.title);
}
