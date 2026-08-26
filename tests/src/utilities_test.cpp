#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "utilities/fileutils.h"
#include "utilities/audioanalysis.h"
#include "collection/collectiongrouping.h"
#include "core/oauthenticator.h"
#include "device/devicemanager.h"
#include "equalizer/equalizer.h"
#include "organize/organize.h"
#include "analyzer/analyzer.h"

#include <algorithm>
#include <cstdint>
#include <glib.h>
#include <unistd.h>
#include <gtest/gtest.h>

TEST(TimeUtils, PrettyTime) {
  EXPECT_EQ("0:00", Utilities::PrettyTime(0));
  EXPECT_EQ("1:01", Utilities::PrettyTime(61));
  EXPECT_EQ("1:01:01", Utilities::PrettyTime(3661));
}

TEST(TimeUtils, PrettyTimeNanosec) {
  EXPECT_EQ("0:05", Utilities::PrettyTimeNanosec(5000000000LL));
}

TEST(StrUtils, SplitJoin) {
  const auto parts = StrUtils::Split("a,b,c", ',');
  ASSERT_EQ(3u, parts.size());
  EXPECT_EQ("a,b,c", StrUtils::Join(parts, ","));
}

TEST(StrUtils, CaseAndTrim) {
  EXPECT_EQ("hello", StrUtils::ToLower("HeLLo"));
  EXPECT_EQ("hello", StrUtils::Trim("  hello\n"));
  EXPECT_TRUE(StrUtils::StartsWith("strawberry", "straw"));
  EXPECT_TRUE(StrUtils::ContainsInsensitive("Album Artist", "artist"));
}

TEST(StrUtils, JsonEscape) {
  EXPECT_EQ("say \\\"hi\\\"", StrUtils::JsonEscape("say \"hi\""));
  EXPECT_EQ("a\\\\b", StrUtils::JsonEscape("a\\b"));
  EXPECT_EQ("line\\nbreak", StrUtils::JsonEscape("line\nbreak"));
}

TEST(FileUtils, BaseAndExtension) {
  EXPECT_EQ("song.mp3", FileUtils::BaseName("/tmp/music/song.mp3"));
  EXPECT_EQ("mp3", FileUtils::Extension("/tmp/music/song.mp3"));
}

TEST(FileUtils, CopyAndRemove) {
  const std::string src = "/tmp/strawberry-copy-src.txt";
  const std::string dest = "/tmp/strawberry-copy-dest.txt";
  ASSERT_TRUE(FileUtils::WriteFile(src, "hello"));
  EXPECT_TRUE(FileUtils::CopyFile(src, dest));
  EXPECT_EQ("hello", FileUtils::ReadFile(dest));
  EXPECT_TRUE(FileUtils::Remove(dest));
  EXPECT_TRUE(FileUtils::Remove(src));
  EXPECT_FALSE(FileUtils::Exists(dest));
}

TEST(OrganizeFormat, ExpandsTokens) {
  OrganizeFormat format("%albumartist/%album/%track - %title");
  Song song;
  song.set_albumartist("Artist");
  song.set_album("Album");
  song.set_title("Title");
  song.set_track(3);
  EXPECT_EQ("Artist/Album/03 - Title", format.GetFilenameForSong(song));
}

TEST(OrganizeFormat, OptionalBlocks) {
  OrganizeFormat format("%albumartist/%album{ - Disc %disc}/{%track - }%title");
  Song with_disc;
  with_disc.set_albumartist("Artist");
  with_disc.set_album("Album");
  with_disc.set_title("Title");
  with_disc.set_track(3);
  with_disc.set_disc(2);
  EXPECT_EQ("Artist/Album - Disc 2/03 - Title", format.GetFilenameForSong(with_disc));
  Song without;
  without.set_albumartist("Artist");
  without.set_album("Album");
  without.set_title("Title");
  EXPECT_EQ("Artist/Album/Title", format.GetFilenameForSong(without));
  EXPECT_FALSE(OrganizeFormat::TokenHasValue("%disc", without));
}

TEST(Organize, ReportsMissingSource) {
  Song song;
  song.set_title("Missing");
  song.set_url("file:///tmp/does-not-exist-strawberry-organize.flac");
  song.set_valid(true);
  const auto errors = Organize().Copy({song}, "/tmp", OrganizeFormat("%title"), false);
  ASSERT_EQ(1u, errors.size());
  EXPECT_NE(std::string::npos, errors[0].message.find("missing"));
}

TEST(CollectionGrouping, DisplayAndKeys) {
  Song song;
  song.set_albumartist("Fleet Foxes");
  song.set_album("Helplessness Blues");
  song.set_year(2011);
  song.set_disc(1);
  song.set_track(2);
  song.set_title("Montezuma");
  song.set_genre("Folk");
  song.set_filetype(Song::FileType::FLAC);
  EXPECT_EQ("Fleet Foxes", CollectionGrouping::DisplayText(CollectionGrouping::GroupBy::AlbumArtist, song));
  EXPECT_EQ("2011 - Helplessness Blues", CollectionGrouping::DisplayText(CollectionGrouping::GroupBy::YearAlbum, song));
  EXPECT_EQ("Helplessness Blues - (Disc 1)", CollectionGrouping::DisplayText(CollectionGrouping::GroupBy::AlbumDisc, song));
  EXPECT_EQ("Unknown", CollectionGrouping::TextOrUnknown({}));
  EXPECT_EQ("Folk", CollectionGrouping::DisplayText(CollectionGrouping::GroupBy::Genre, song));
  bool unique = false;
  const std::string key = CollectionGrouping::ContainerKey(CollectionGrouping::GroupBy::Album, song, &unique, false);
  EXPECT_NE(std::string::npos, key.find("Helplessness Blues"));
  EXPECT_TRUE(CollectionGrouping::IsAlbumGroupBy(CollectionGrouping::GroupBy::YearAlbumDisc));
  EXPECT_EQ(CollectionGrouping::GroupBy::AlbumArtist, CollectionGrouping::FromLegacy("artist-album").first);
  EXPECT_EQ(2, CollectionGrouping::ComboIndex(CollectionGrouping::GroupBy::AlbumArtist));
}

TEST(CollectionGrouping, BuildTreeThreeLevels) {
  Song a;
  a.set_albumartist("A");
  a.set_album("One");
  a.set_title("First");
  a.set_track(1);
  a.set_valid(true);
  Song b;
  b.set_albumartist("A");
  b.set_album("Two");
  b.set_title("Second");
  b.set_track(1);
  b.set_valid(true);
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  const auto tree = CollectionGrouping::BuildTree({a, b}, grouping, false, false, false);
  ASSERT_EQ(1u, tree.children.size());
  EXPECT_EQ("A", tree.children[0].display);
  ASSERT_EQ(2u, tree.children[0].children.size());
}

TEST(CollectionGrouping, SavedRoundTrip) {
  CollectionGrouping::Grouping grouping{CollectionGrouping::GroupBy::Genre, CollectionGrouping::GroupBy::Year, CollectionGrouping::GroupBy::Album};
  CollectionGrouping::AddSaved("GTK test grouping", grouping);
  const auto saved = CollectionGrouping::LoadSaved();
  bool found = false;
  for (const auto &entry : saved) {
    if (entry.first == "GTK test grouping") {
      found = entry.second == grouping;
    }
  }
  EXPECT_TRUE(found);
  CollectionGrouping::RemoveSaved("GTK test grouping");
}

TEST(Equalizer, BuiltinPresets) {
  const auto names = Equalizer::BuiltinPresetNames();
  EXPECT_NE(names.end(), std::find(names.begin(), names.end(), "Rock"));
  EXPECT_NE(names.end(), std::find(names.begin(), names.end(), "Techno"));
  EXPECT_NE(names.end(), std::find(names.begin(), names.end(), "Laptop/Headphones"));
  EXPECT_GE(names.size(), 14u);
  Equalizer eq;
  eq.LoadPreset("Rock");
  EXPECT_NE(0, eq.gains()[0]);
}

TEST(Analyzer, Types) {
  const auto types = Analyzer::Types();
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Bar"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Rainbow"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Turbine"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Wave"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Sonic"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Block"));
}

TEST(AudioAnalysis, PeaksMoodAndScope) {
  int16_t samples[8] = {0, 32767, 0, -16384, 1000, 2000, 3000, 0};
  const auto peaks = AudioAnalysis::PeaksFromPcm(samples, 8, 1, 4);
  ASSERT_EQ(4u, peaks.size());
  EXPECT_GT(peaks[0], 0.9f);
  EXPECT_NEAR(peaks[1], 0.5f, 0.02f);
  const auto mood = AudioAnalysis::MoodFromPeaks(peaks);
  ASSERT_EQ(12u, mood.size());
  EXPECT_GT(mood[0], mood[3]);
  const auto scope = AudioAnalysis::ScopeFromMagnitudes({0.0f, -40.0f, -80.0f});
  ASSERT_EQ(3u, scope.size());
  EXPECT_GT(scope[0], scope[1]);
  EXPECT_EQ(0, scope[2]);
  EXPECT_TRUE(AudioAnalysis::DecodePcm({}).empty());
}

TEST(Analyzer, MagnitudesFillBands) {
  Analyzer analyzer;
  analyzer.SetMagnitudes(std::vector<float>(64, -20.0f));
  EXPECT_FALSE(analyzer.bands().empty());
  EXPECT_GT(analyzer.bands().front(), 0.0f);
}

TEST(FileUtils, ListDirectoryRecursive) {
  const std::string root = "/tmp/strawberry-recursive-" + std::to_string(getpid());
  const std::string nested = FileUtils::Join(root, "album");
  g_mkdir_with_parents(nested.c_str(), 0755);
  ASSERT_TRUE(FileUtils::WriteFile(FileUtils::Join(nested, "track.flac"), "x"));
  ASSERT_TRUE(FileUtils::WriteFile(FileUtils::Join(root, "skip.txt"), "x"));
  const auto files = FileUtils::ListDirectoryRecursive(root);
  EXPECT_NE(files.end(), std::find(files.begin(), files.end(), FileUtils::Join(nested, "track.flac")));
  FileUtils::Remove(FileUtils::Join(nested, "track.flac"));
  FileUtils::Remove(FileUtils::Join(root, "skip.txt"));
  rmdir(nested.c_str());
  rmdir(root.c_str());
}

TEST(DeviceManager, SongsFromDirectoryAndCdda) {
  const std::string root = "/tmp/strawberry-device-" + std::to_string(getpid());
  g_mkdir_with_parents(root.c_str(), 0755);
  ASSERT_TRUE(FileUtils::WriteFile(FileUtils::Join(root, "song.mp3"), "x"));
  const SongList files = DeviceManager::SongsFromDirectory(root);
  ASSERT_EQ(1u, files.size());
  EXPECT_EQ(Song::Source::Device, files[0].source());
  EXPECT_TRUE(files[0].is_valid());
  FileUtils::Remove(FileUtils::Join(root, "song.mp3"));
  rmdir(root.c_str());

  const SongList cd = DeviceManager::MakeCddaSongs(1, 2, {180000000000LL, 200000000000LL});
  ASSERT_EQ(2u, cd.size());
  EXPECT_EQ("cdda://1", cd[0].url());
  EXPECT_EQ(Song::Source::CDDA, cd[0].source());
  EXPECT_EQ(180000000000LL, cd[0].length_nanosec());
  EXPECT_TRUE(DeviceManager::MakeCddaSongs(0, 0, {}).empty());
}

TEST(OAuthenticator, BuildAuthorizeUrl) {
  const std::string url = OAuthenticator::BuildAuthorizeUrl("https://example.com/oauth", "client", "http://127.0.0.1:9/callback", "scope");
  EXPECT_NE(std::string::npos, url.find("response_type=code"));
  EXPECT_NE(std::string::npos, url.find("client_id=client"));
  EXPECT_NE(std::string::npos, url.find("redirect_uri="));
}

TEST(OAuthenticator, ClientCredentialsBodyAndToken) {
  const std::string body = OAuthenticator::ClientCredentialsBody("id", "secret");
  EXPECT_NE(std::string::npos, body.find("grant_type=client_credentials"));
  EXPECT_NE(std::string::npos, body.find("client_id=id"));
  EXPECT_NE(std::string::npos, body.find("client_secret=secret"));
  EXPECT_EQ("Basic aWQ6c2VjcmV0", OAuthenticator::BasicAuthorizationHeader("id", "secret"));
  EXPECT_EQ("tok", OAuthenticator::ParseAccessToken(R"json({"access_token":"tok","token_type":"Bearer"})json"));
}
