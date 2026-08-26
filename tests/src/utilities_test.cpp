#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "utilities/fileutils.h"
#include "utilities/audioanalysis.h"
#include "collection/collectiongrouping.h"
#include "core/oauthenticator.h"
#include "device/cddasongloader.h"
#include "device/devicemanager.h"
#include "device/filesystemdevice.h"
#include "device/giolister.h"
#include "equalizer/equalizer.h"
#include "constants/filefilterconstants.h"
#include "context/contextalbum.h"
#include "context/contexttechnical.h"
#include "organize/organize.h"
#include "organize/organizeformatvalidator.h"
#include "analyzer/analyzer.h"
#include "analyzer/fht.h"
#include "constants/behavioursettings.h"
#include "context/contextformattokens.h"
#include "core/appearance.h"
#include "core/deletefiles.h"
#include "core/enginemetadata.h"
#include "core/windowgeometry.h"
#include "core/filesystemmusicstorage.h"
#include "core/memorydatabase.h"
#include "core/scopedtransaction.h"
#include "core/temporaryfile.h"
#include "device/cddalister.h"
#include "device/udisks2lister.h"
#include "engine/alsadevicefinder.h"
#include "engine/chromaprinter.h"
#include "engine/devicefinders.h"
#include "engine/ebur128analysis.h"
#include "engine/enginedevice.h"
#include "engine/gststartup.h"
#include "moodbar/moodbarbuilder.h"
#include "transcoder/transcoder.h"
#include "waveform/waveformbuilder.h"
#include "transcoder/transcoderoptionsinterface.h"
#include "widgets/playingwidget.h"
#include "widgets/stretchheaderview.h"

#include <algorithm>
#include <cstdint>
#include <glib.h>
#include <glib/gstdio.h>
#include <gst/gst.h>
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

TEST(FileUtils, FileSizeAndMtime) {
  const std::string path = "/tmp/strawberry-filesize.txt";
  FileUtils::WriteFile(path, "hello");
  EXPECT_EQ(5, FileUtils::FileSize(path));
  EXPECT_GT(FileUtils::FileMtime(path), 0);
  EXPECT_EQ(-1, FileUtils::FileSize("/tmp/strawberry-missing-filesize.txt"));
  FileUtils::Remove(path);
}

TEST(FileUtils, PrettySize) {
  EXPECT_EQ("0 B", FileUtils::PrettySize(0));
  EXPECT_EQ("512 B", FileUtils::PrettySize(512));
  EXPECT_EQ("2 KB", FileUtils::PrettySize(2048));
  EXPECT_FALSE(FileUtils::PrettySize(2 * 1024 * 1024).empty());
  EXPECT_TRUE(FileUtils::PrettySize(-1).empty());
}

TEST(FileUtils, FreeAndTotalSpace) {
  EXPECT_GE(FileUtils::FreeSpaceBytes("/tmp"), 0);
  EXPECT_GE(FileUtils::TotalSpaceBytes("/tmp"), FileUtils::FreeSpaceBytes("/tmp"));
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

TEST(ContextTechnical, RowsAndFormats) {
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_bitrate(320);
  song.set_samplerate(44100);
  song.set_bitdepth(16);
  song.set_length_nanosec(30000000000LL);
  song.set_filetype(Song::FileType::FLAC);
  song.set_valid(true);
  const auto rows = ContextTechnical::Rows(song);
  ASSERT_EQ(5u, rows.size());
  EXPECT_EQ("Filetype", rows[0].first);
  EXPECT_EQ("FLAC", rows[0].second);
  EXPECT_EQ("Length", rows[1].first);
  EXPECT_EQ("Samplerate", rows[2].first);
  EXPECT_EQ("44100 Hz", rows[2].second);
  EXPECT_EQ("Bit depth", rows[3].first);
  EXPECT_EQ("16 Bit", rows[3].second);
  EXPECT_EQ("Bitrate", rows[4].first);
  EXPECT_EQ("320 kbps", rows[4].second);
  EXPECT_EQ("Roads - Portishead", ContextTechnical::Headline(song, {}));
  EXPECT_EQ("Dummy", ContextTechnical::Summary(song, {}));
  EXPECT_EQ("Portishead", ContextTechnical::Headline(song, "%artist%"));
  EXPECT_TRUE(ContextTechnical::Rows(Song()).empty());
}

TEST(ContextAlbum, ImagePathExtensions) {
  EXPECT_TRUE(ContextAlbum::IsImagePath("/tmp/cover.jpg"));
  EXPECT_TRUE(ContextAlbum::IsImagePath("https://ex.com/a.JPEG"));
  EXPECT_TRUE(ContextAlbum::IsImagePath("art.webp"));
  EXPECT_TRUE(ContextAlbum::IsImagePath("folder/cover.bmp"));
  EXPECT_TRUE(ContextAlbum::IsImagePath("art.gif"));
  EXPECT_FALSE(ContextAlbum::IsImagePath("/tmp/song.flac"));
  EXPECT_FALSE(ContextAlbum::IsImagePath("cover"));
}

TEST(ContextAlbum, FadeOpacitiesMatchQtTimeline) {
  EXPECT_DOUBLE_EQ(0.0, ContextAlbum::FadeInOpacity(0));
  EXPECT_DOUBLE_EQ(1.0, ContextAlbum::FadeOutOpacity(0));
  EXPECT_DOUBLE_EQ(0.5, ContextAlbum::FadeInOpacity(ContextAlbum::kFadeTimelineMs / 2));
  EXPECT_DOUBLE_EQ(0.5, ContextAlbum::FadeOutOpacity(ContextAlbum::kFadeTimelineMs / 2));
  EXPECT_DOUBLE_EQ(1.0, ContextAlbum::FadeInOpacity(ContextAlbum::kFadeTimelineMs));
  EXPECT_DOUBLE_EQ(0.0, ContextAlbum::FadeOutOpacity(ContextAlbum::kFadeTimelineMs));
  EXPECT_DOUBLE_EQ(1.0, ContextAlbum::FadeInOpacity(ContextAlbum::kFadeTimelineMs + 50));
}

TEST(ContextTechnical, CollectionTotalsMatchQtSingularPlural) {
  EXPECT_EQ("0 songs\n0 artists\n0 albums", ContextTechnical::Totals(0, 0, 0));
  EXPECT_EQ("1 song\n1 artist\n1 album", ContextTechnical::Totals(1, 1, 1));
  EXPECT_EQ("12 songs\n3 artists\n4 albums", ContextTechnical::Totals(12, 3, 4));
}

TEST(FileFilterConstants, QtAudioPlaylistAndImageGlobs) {
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kFileFilter, "flac"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kFileFilter, "dsf"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kFileFilter, "m3u8"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kFileFilter, "cue"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kFileFilter, "mod"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kPlaylist, "xspf"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kLoadImages, "webp"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kLoadImages, "bmp"));
  EXPECT_TRUE(FileFilterConstants::ContainsExtension(FileFilterConstants::kSaveImages, "png"));
  EXPECT_FALSE(FileFilterConstants::ContainsExtension(FileFilterConstants::kSaveImages, "gif"));
  EXPECT_TRUE(FileFilterConstants::PathMatchesGlobs("album.m3u8", FileFilterConstants::kPlaylist));
  EXPECT_FALSE(FileFilterConstants::PathMatchesGlobs("song.flac", FileFilterConstants::kPlaylist));
}

TEST(OrganizeFormatValidator, EmptyAndUnbalanced) {
  std::string error;
  EXPECT_FALSE(OrganizeFormatValidator::IsValid("", &error));
  EXPECT_EQ("Format is empty", error);
  EXPECT_FALSE(OrganizeFormatValidator::IsValid("%artist/{%title", &error));
  EXPECT_FALSE(error.empty());
  EXPECT_TRUE(OrganizeFormatValidator::IsValid("%artist/%title"));
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

TEST(OrganizeFormat, ExtraTokensArtistInitialAndExtension) {
  Song song;
  song.set_albumartist("The Beatles");
  song.set_album("Abbey Road");
  song.set_title("Come Together");
  song.set_track(1);
  song.set_bitrate(320);
  song.set_samplerate(44100);
  song.set_bitdepth(16);
  song.set_length_nanosec(4 * 1000000000LL);
  song.set_url("file:///tmp/come-together.flac");
  song.set_lyrics("here come old flat top");
  EXPECT_EQ("B", OrganizeFormat::ArtistInitial("The Beatles"));
  EXPECT_EQ("P", OrganizeFormat::ArtistInitial("portishead"));
  EXPECT_EQ("B", OrganizeFormat::TokenValue("%artistinitial", song));
  EXPECT_EQ("flac", OrganizeFormat::TokenValue("%extension", song));
  EXPECT_EQ("320", OrganizeFormat::TokenValue("%bitrate", song));
  EXPECT_EQ("44100", OrganizeFormat::TokenValue("%samplerate", song));
  EXPECT_EQ("16", OrganizeFormat::TokenValue("%bitdepth", song));
  EXPECT_EQ("4", OrganizeFormat::TokenValue("%length", song));
  EXPECT_EQ("here come old flat top", OrganizeFormat::TokenValue("%lyrics", song));
  EXPECT_EQ("B/Abbey Road/01 - Come Together.flac",
            OrganizeFormat("%artistinitial/%album/{%track - }%title").GetFilenameForSong(song));
  song.set_compilation(true);
  EXPECT_EQ("Various Artists", OrganizeFormat::TokenValue("%albumartist", song));
}

TEST(OrganizeFormat, ReplaceSpacesAndEmptyFallback) {
  OrganizeFormat format("%album/%title");
  format.set_replace_spaces(true);
  Song song;
  song.set_album("Helplessness Blues");
  song.set_title("The Shrine / An Argument");
  EXPECT_EQ("Helplessness_Blues/The_Shrine___An_Argument", format.GetFilenameForSong(song));
  Song empty;
  empty.set_basefilename("fallback.ogg");
  EXPECT_EQ("fallback.ogg", OrganizeFormat("%title").GetFilenameForSong(empty));
}

TEST(Organize, OverwriteAndAlbumCover) {
  char dir_template[] = "/tmp/strawberry-organize-XXXXXX";
  const std::string dir = mkdtemp(dir_template);
  const std::string src_dir = FileUtils::Join(dir, "src");
  const std::string dest_dir = FileUtils::Join(dir, "dest");
  g_mkdir_with_parents(src_dir.c_str(), 0755);
  g_mkdir_with_parents(dest_dir.c_str(), 0755);
  const std::string src = FileUtils::Join(src_dir, "song.flac");
  const std::string cover = FileUtils::Join(src_dir, "cover.jpg");
  const std::string dest = FileUtils::Join(dest_dir, "Title.flac");
  ASSERT_TRUE(FileUtils::WriteFile(src, "audio-v1"));
  ASSERT_TRUE(FileUtils::WriteFile(cover, "cover-bytes"));
  ASSERT_TRUE(FileUtils::WriteFile(dest, "old"));

  Song song;
  song.set_valid(true);
  song.set_title("Title");
  song.set_url(FileUtils::UriFromPath(src));
  EXPECT_EQ(cover, Organize::CoverPathForSong(song));

  OrganizeFormat format("%title");
  const auto skipped = Organize().Copy({song}, dest_dir, format, Organize::Options{});
  ASSERT_EQ(1u, skipped.size());
  EXPECT_NE(std::string::npos, skipped[0].message.find("exists"));
  EXPECT_EQ("old", FileUtils::ReadFile(dest));

  Organize::Options options;
  options.overwrite = true;
  options.albumcover = true;
  const auto copied = Organize().Copy({song}, dest_dir, format, options);
  EXPECT_TRUE(copied.empty());
  EXPECT_EQ("audio-v1", FileUtils::ReadFile(dest));
  EXPECT_EQ("cover-bytes", FileUtils::ReadFile(FileUtils::Join(dest_dir, "cover.jpg")));

  FileUtils::Remove(src);
  FileUtils::Remove(cover);
  FileUtils::Remove(dest);
  FileUtils::Remove(FileUtils::Join(dest_dir, "cover.jpg"));
  rmdir(src_dir.c_str());
  rmdir(dest_dir.c_str());
  rmdir(dir.c_str());
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
  EXPECT_EQ(-100, Equalizer::ClampBalance(-250));
  EXPECT_EQ(100, Equalizer::ClampBalance(250));
  EXPECT_EQ(0, Equalizer::ClampBalance(0));
}

TEST(Analyzer, Types) {
  const auto types = Analyzer::Types();
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Bar"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Rainbow"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "RainbowDash"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "NyanCat"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Turbine"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Wave"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Sonic"));
  EXPECT_NE(types.end(), std::find(types.begin(), types.end(), "Block"));
  EXPECT_EQ(5, Analyzer::ClampFramerate(1));
  EXPECT_EQ(60, Analyzer::ClampFramerate(120));
  EXPECT_EQ("Rainbow", Analyzer::NextType("Bar"));
  EXPECT_EQ("Bar", Analyzer::NextType("Block"));
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

TEST(FHT, AppliesHannWindow) {
  FHT fht(4);
  ASSERT_EQ(4, fht.size());
  ASSERT_EQ(4u, fht.window().size());
  EXPECT_NEAR(0.0f, fht.window().front(), 0.001f);
  EXPECT_NEAR(1.0f, fht.window()[2], 0.001f);
  std::vector<float> samples = {1.0f, 1.0f, 1.0f, 1.0f};
  fht.Transform(&samples);
  EXPECT_NEAR(0.0f, samples.front(), 0.001f);
  EXPECT_NEAR(1.0f, samples[2], 0.001f);
}

TEST(Transcoder, PresetAndPipelineFor) {
  EXPECT_EQ("mp3", Transcoder::Extension(Transcoder::Format::MP3));
  EXPECT_EQ("FLAC", Transcoder::FormatName(Transcoder::Format::FLAC));
  const Transcoder::Preset flac = Transcoder::PresetFor(Transcoder::Format::FLAC);
  EXPECT_EQ("flac", flac.extension);
  EXPECT_EQ("audio/x-flac", flac.codec_mimetype);
  const std::string mp3 = Transcoder::PipelineFor(Transcoder::Format::MP3, 5);
  EXPECT_NE(std::string::npos, mp3.find("lamemp3enc"));
  EXPECT_NE(std::string::npos, mp3.find("xingmux"));
  const std::string opus = Transcoder::PipelineFor(Transcoder::Format::Opus, 5);
  EXPECT_NE(std::string::npos, opus.find("opusenc"));
  EXPECT_NE(std::string::npos, opus.find("bitrate="));
  const std::string aac = Transcoder::PipelineFor(Transcoder::Format::AAC, 5);
  EXPECT_NE(std::string::npos, aac.find("avenc_aac"));
  EXPECT_NE(std::string::npos, aac.find("mp4mux"));
  const std::string speex = Transcoder::PipelineFor(Transcoder::Format::Speex, 8);
  EXPECT_NE(std::string::npos, speex.find("speexenc quality=8"));
  const std::string wavpack = Transcoder::PipelineFor(Transcoder::Format::WavPack, 9);
  EXPECT_NE(std::string::npos, wavpack.find("wavpackenc mode=2"));
  const std::string asf = Transcoder::PipelineFor(Transcoder::Format::ASF, 5);
  EXPECT_NE(std::string::npos, asf.find("avenc_wmav2"));
  EXPECT_EQ(192, TranscoderOptionsInterface::BitrateKbps(5, 64, 320));
  Transcoder transcoder;
  transcoder.set_quality(8);
  EXPECT_EQ(8, transcoder.quality());
  EXPECT_EQ(0, transcoder.job_count());
}

TEST(CddaSongLoader, SongsDelegateToDeviceManager) {
  const SongList songs = CddaSongLoader::Songs(1, 2, {1000, 2000});
  ASSERT_EQ(2u, songs.size());
  EXPECT_EQ("cdda://1", songs[0].url());
  EXPECT_EQ(Song::Source::CDDA, songs[0].source());
  EXPECT_EQ(1000, songs[0].length_nanosec());
  EXPECT_TRUE(CddaSongLoader::Songs(0, 0, {}).empty());
}

TEST(FilesystemDevice, SongsFromMountedDirectory) {
  const std::string root = "/tmp/strawberry-fsdevice-" + std::to_string(getpid());
  g_mkdir_with_parents(root.c_str(), 0755);
  ASSERT_TRUE(FileUtils::WriteFile(FileUtils::Join(root, "song.mp3"), "x"));
  ConnectedDevice info;
  info.mount_path = root;
  info.backend = "gio";
  FilesystemDevice device(info);
  const SongList songs = device.Songs();
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ(Song::Source::Device, songs[0].source());
  FileUtils::Remove(FileUtils::Join(root, "song.mp3"));
  rmdir(root.c_str());
}

TEST(GioLister, BackendName) {
  GioLister lister;
  EXPECT_EQ("gio", lister.backend());
}

TEST(DeviceFinders, OutputsAndDefaultDevice) {
  DeviceFinders finders;
  finders.Init();
  const auto outputs = finders.Outputs();
  EXPECT_NE(outputs.end(), std::find(outputs.begin(), outputs.end(), "autoaudiosink"));
  EXPECT_NE(outputs.end(), std::find(outputs.begin(), outputs.end(), "alsasink"));
  ASSERT_FALSE(finders.ListDevices().empty());
  EXPECT_EQ("Default", finders.ListDevices().front().description);
  AlsaDeviceFinder alsa;
  EXPECT_EQ("alsa", alsa.name());
  EXPECT_TRUE(alsa.Initialize());
}

TEST(EngineDevice, GuessIconName) {
  EngineDevice hdmi;
  hdmi.description = "HDMI output";
  EXPECT_EQ("video-display-symbolic", hdmi.GuessIconName());
  EngineDevice usb;
  usb.description = "USB Headset";
  EXPECT_EQ("audio-headphones-symbolic", usb.GuessIconName());
}

TEST(GstStartup, InitializeIsIdempotent) {
  GstStartup::Initialize();
  GstStartup::Initialize();
  EXPECT_TRUE(gst_is_initialized());
}

TEST(Chromaprinter, EmptyUrlReportsError) {
  Chromaprinter printer("");
  EXPECT_TRUE(printer.CreateFingerprint().empty());
  EXPECT_FALSE(printer.LastError().empty());
}

TEST(EBUR128Analysis, EmptySongIsNullopt) {
  EXPECT_FALSE(EBUR128Analysis::Compute(Song()).has_value());
}

TEST(MoodbarBuilder, FromPcm) {
  const int16_t samples[] = {0, 32767, 0, -16384};
  const auto mood = MoodbarBuilder::FromPcm(samples, 4, 1, 2);
  ASSERT_EQ(6u, mood.size());
}

TEST(WaveformBuilder, FromPcm) {
  const int16_t samples[] = {0, 32767, 0, -16384};
  const auto peaks = WaveformBuilder::FromPcm(samples, 4, 1, 2);
  ASSERT_EQ(2u, peaks.size());
  EXPECT_GT(peaks[0], 0.9f);
}

TEST(FilesystemMusicStorage, CopyAndDelete) {
  TemporaryFile tmp("strawberry-src-XXXXXX");
  ASSERT_FALSE(tmp.filename().empty());
  ASSERT_TRUE(FileUtils::WriteFile(tmp.filename(), "abc"));
  const std::string dest_dir = "/tmp/strawberry-storage-" + std::to_string(getpid());
  g_mkdir_with_parents(dest_dir.c_str(), 0755);
  FilesystemMusicStorage storage(dest_dir);
  MusicStorage::CopyJob job;
  job.source = tmp.filename();
  job.destination = FileUtils::Join(dest_dir, "copied.txt");
  std::string error;
  EXPECT_TRUE(storage.CopyToStorage(job, error));
  EXPECT_EQ("abc", FileUtils::ReadFile(job.destination));
  Song song;
  song.set_url(FileUtils::UriFromPath(job.destination));
  EXPECT_TRUE(storage.DeleteFromStorage({song, false}));
  EXPECT_FALSE(FileUtils::Exists(job.destination));
  rmdir(dest_dir.c_str());
}

TEST(DeleteFiles, ReportsMissingFiles) {
  TaskManager tasks;
  FilesystemMusicStorage storage("/tmp");
  DeleteFiles deleter(&tasks, &storage, false);
  Song missing;
  missing.set_url("file:///tmp/does-not-exist-strawberry.flac");
  deleter.Start(SongList{missing});
  ASSERT_EQ(1u, deleter.errors().size());
}

TEST(MemoryDatabase, OpensAndScopedTransaction) {
  MemoryDatabase db;
  ASSERT_TRUE(db.Open());
  ScopedTransaction tx(&db);
  EXPECT_TRUE(db.Exec("CREATE TABLE IF NOT EXISTS t(id INTEGER)"));
  tx.Commit();
}

TEST(EngineMetadata, ToSong) {
  EngineMetadata meta;
  meta.title = "Roads";
  meta.artist = "Portishead";
  meta.length_nanosec = 1000;
  const Song song = meta.ToSong(Song::Source::LocalFile);
  EXPECT_EQ("Roads", song.title());
  EXPECT_EQ("Portishead", song.artist());
  EXPECT_EQ(Song::Source::LocalFile, song.source());
}

TEST(StretchHeaderView, StretchColumnTakesRemainder) {
  StretchHeaderView header;
  header.SetColumns({"A", "B", "C"});
  header.SetStretchColumn(1);
  EXPECT_NEAR(96.0, header.ColumnWidth(0, 400), 0.01);
  EXPECT_NEAR(208.0, header.ColumnWidth(1, 400), 0.01);
}

TEST(CddaAndUdisksListers, BackendNames) {
  EXPECT_EQ("cdda", CddaLister().backend());
  EXPECT_EQ("udisks2", Udisks2Lister().backend());
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

TEST(DeviceManager, ForgetHidesDeviceFromRescan) {
  DeviceManager manager;
  manager.Rescan();
  if (manager.devices().empty()) {
    EXPECT_FALSE(manager.Forget(""));
    return;
  }
  const std::string id = manager.devices().front().unique_id;
  EXPECT_TRUE(manager.Forget(id));
  for (const ConnectedDevice &device : manager.devices()) {
    EXPECT_NE(id, device.unique_id);
  }
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

TEST(OAuthenticator, ParseTokenResponseAndRefreshBody) {
  const auto token = OAuthenticator::ParseTokenResponse(R"json({"access_token":"a","refresh_token":"r","token_type":"Bearer","expires_in":3600})json");
  EXPECT_EQ("a", token.access_token);
  EXPECT_EQ("r", token.refresh_token);
  EXPECT_EQ("Bearer", token.token_type);
  EXPECT_EQ(3600, token.expires_in);
  const std::string body = OAuthenticator::RefreshTokenBody("refresh", "id", "secret");
  EXPECT_NE(std::string::npos, body.find("grant_type=refresh_token"));
  EXPECT_NE(std::string::npos, body.find("refresh_token=refresh"));
  EXPECT_NE(std::string::npos, body.find("client_id=id"));
}

TEST(OAuthenticator, AccessTokenExpiredUsesSkew) {
  EXPECT_FALSE(OAuthenticator::AccessTokenExpired(1000, 3600, 1000));
  EXPECT_FALSE(OAuthenticator::AccessTokenExpired(1000, 3600, 4539));
  EXPECT_TRUE(OAuthenticator::AccessTokenExpired(1000, 3600, 4540));
  EXPECT_TRUE(OAuthenticator::AccessTokenExpired(1000, 3600, 5000));
  EXPECT_FALSE(OAuthenticator::AccessTokenExpired(0, 3600, 5000));
  EXPECT_FALSE(OAuthenticator::AccessTokenExpired(1000, 0, 5000));
}

TEST(DeviceFinders, ChoiceKeyAndOutputLabels) {
  EXPECT_EQ("pulsesink|alsa_output.pci", DeviceFinders::ChoiceKey("pulsesink", "alsa_output.pci"));
  std::string output;
  std::string device;
  DeviceFinders::SplitChoiceKey("alsasink|hw:0,0", &output, &device);
  EXPECT_EQ("alsasink", output);
  EXPECT_EQ("hw:0,0", device);
  DeviceFinders::SplitChoiceKey("pipewiresink|", &output, &device);
  EXPECT_EQ("pipewiresink", output);
  EXPECT_TRUE(device.empty());
  DeviceFinders::SplitChoiceKey("autoaudiosink", &output, &device);
  EXPECT_EQ("autoaudiosink", output);
  EXPECT_TRUE(device.empty());
  EXPECT_EQ("Automatic", DeviceFinders::OutputLabel("autoaudiosink"));
  EXPECT_EQ("PulseAudio", DeviceFinders::OutputLabel("pulsesink"));
  EXPECT_EQ("PipeWire", DeviceFinders::OutputLabel("pipewiresink"));
  EXPECT_EQ("ALSA", DeviceFinders::OutputLabel("alsasink"));

  DeviceFinders finders;
  finders.Init();
  const auto outputs = finders.Outputs();
  EXPECT_NE(outputs.end(), std::find(outputs.begin(), outputs.end(), "autoaudiosink"));
  EXPECT_FALSE(finders.ListDevices().empty());
}

TEST(WindowGeometry, ClampsAndMapsStartup) {
  const WindowGeometry::State huge = WindowGeometry::FromValues(20000, 10, true);
  EXPECT_EQ(WindowGeometry::kMaxWidth, huge.width);
  EXPECT_EQ(WindowGeometry::kMinHeight, huge.height);
  EXPECT_TRUE(huge.maximized);
  const WindowGeometry::State fallback = WindowGeometry::FromValues(0, 0, false);
  EXPECT_EQ(WindowGeometry::kDefaultWidth, fallback.width);
  EXPECT_EQ(WindowGeometry::kDefaultHeight, fallback.height);
  EXPECT_EQ(4, WindowGeometry::StartupAction(static_cast<int>(BehaviourSettings::StartupBehaviour::Remember), true));
  EXPECT_EQ(2, WindowGeometry::StartupAction(static_cast<int>(BehaviourSettings::StartupBehaviour::Remember), false));
  EXPECT_EQ(3, WindowGeometry::StartupAction(static_cast<int>(BehaviourSettings::StartupBehaviour::Hide), false));
  EXPECT_EQ(5, WindowGeometry::StartupAction(static_cast<int>(BehaviourSettings::StartupBehaviour::ShowMinimized), true));
}

TEST(Appearance, BackgroundCssForTypesAndUrls) {
  EXPECT_EQ("file:///tmp/cover.jpg", Appearance::CssUrl("/tmp/cover.jpg"));
  EXPECT_EQ("https://example/a.png", Appearance::CssUrl("https://example/a.png"));
  EXPECT_EQ("top left", Appearance::BackgroundPositionCss(1));
  EXPECT_EQ("bottom right", Appearance::BackgroundPositionCss(5));
  EXPECT_TRUE(Appearance::BuildBackgroundCss(0, {}, 5, 0, 40).empty());
  EXPECT_EQ(".strawberry-main { background-image: none; }", Appearance::BuildBackgroundCss(1, {}, 5, 0, 40));
  EXPECT_NE(std::string::npos, Appearance::BuildBackgroundCss(4, {}, 5, 0, 40).find("#8B1E3F"));
  const std::string custom = Appearance::BuildBackgroundCss(2, "/tmp/wall.jpg", 1, 8, 40);
  EXPECT_NE(std::string::npos, custom.find("file:///tmp/wall.jpg"));
  EXPECT_NE(std::string::npos, custom.find("top left"));
  EXPECT_NE(std::string::npos, custom.find("blur(8px)"));
}

TEST(PlayingWidget, CoverSizeAndFade) {
  EXPECT_EQ(PlayingWidget::kSmallCover, PlayingWidget::CoverSize(PlayingWidget::Mode::SmallSongDetails, true, 400));
  EXPECT_EQ(PlayingWidget::kLargeCover, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, false, 400));
  EXPECT_EQ(320, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, true, 800));
  EXPECT_EQ(80, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, true, 20));
  EXPECT_EQ(0.0, PlayingWidget::FadeInOpacity(0));
  EXPECT_EQ(1.0, PlayingWidget::FadeInOpacity(PlayingWidget::kFadeTimelineMs));
  EXPECT_NEAR(0.5, PlayingWidget::FadeInOpacity(500), 0.001);
  EXPECT_NEAR(0.5, PlayingWidget::FadeOutOpacity(500), 0.001);
}

TEST(ContextFormatTokens, InsertsKnownTokens) {
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%title%"));
  EXPECT_FALSE(ContextFormatTokens::IsKnown("%unknown%"));
  EXPECT_EQ("%title%", ContextFormatTokens::Insert({}, "%title%"));
  EXPECT_EQ("%title%%artist%", ContextFormatTokens::Insert("%title%", "%artist%"));
  EXPECT_EQ("%title%", ContextFormatTokens::Insert("%title%", {}));
  EXPECT_FALSE(ContextFormatTokens::All().empty());
}
