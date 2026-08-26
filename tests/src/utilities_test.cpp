#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "widgets/multiloadingtext.h"
#include "widgets/trackslidertime.h"
#include "widgets/volumesliderwheel.h"
#include "utilities/fileutils.h"
#include "utilities/audioanalysis.h"
#include "collection/collectiongrouping.h"
#include "core/oauthenticator.h"
#include "device/cddasongloader.h"
#include "device/devicemanager.h"
#include "device/filesystemdevice.h"
#include "device/giolister.h"
#include "equalizer/equalizer.h"
#include "equalizer/equalizerpersist.h"
#include "equalizer/equalizerpresets.h"
#include "constants/filefilterconstants.h"
#include "constants/collectionsettings.h"
#include "analyzer/analyzerframerate.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistlook.h"
#include "context/contextalbum.h"
#include "context/contextcover.h"
#include "context/contextfont.h"
#include "context/contextoptions.h"
#include "context/contextidle.h"
#include "context/contextplayingtext.h"
#include "context/contexttechnical.h"
#include "dialogs/addstreamurl.h"
#include "dialogs/deletefilespolicy.h"
#include "dialogs/userpasslabels.h"
#include "organize/organize.h"
#include "organize/organizefilename.h"
#include "organize/organizeformatvalidator.h"
#include "organize/organizepreview.h"
#include "organize/organizetranscode.h"
#include "constants/organizesettings.h"
#include "analyzer/analyzer.h"
#include "analyzer/fht.h"
#include "constants/behavioursettings.h"
#include "desktop/taskbarprogress.h"
#include "context/contextformattokens.h"
#include "constants/appearancesettings.h"
#include "core/appearance.h"
#include "core/appearancecolors.h"
#include "core/deletefiles.h"
#include "core/enginemetadata.h"
#include "core/windowgeometry.h"
#include "core/mainwindowsettings.h"
#include "ui/mainwindowkeyboard.h"
#include "ui/mainwindowlook.h"
#include "ui/mainwindowmenu.h"
#include "widgets/filtersearchkeyboard.h"
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
#include "transcoder/transcoderoptionsfields.h"
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

TEST(TrackSliderTime, DurationTogglesRemainingLikeQt) {
  EXPECT_STREQ("Seekbar", TrackSliderTime::SettingsGroup());
  EXPECT_STREQ("show_remaining", TrackSliderTime::SettingsKey());
  EXPECT_FALSE(TrackSliderTime::DefaultShowRemaining());
  EXPECT_EQ("1:30", TrackSliderTime::PositionLabel(90000000000LL));
  EXPECT_EQ("3:00", TrackSliderTime::DurationLabel(false, 90000000000LL, 180000000000LL));
  EXPECT_EQ("-1:30", TrackSliderTime::DurationLabel(true, 90000000000LL, 180000000000LL));
  EXPECT_EQ("-0:00", TrackSliderTime::DurationLabel(true, 180000000000LL, 180000000000LL));
  EXPECT_EQ("1:30 / -1:30", TrackSliderTime::PopupText(true, 90000000000LL, 180000000000LL));
  EXPECT_STREQ("Click to toggle remaining time", TrackSliderTime::DurationTooltip());
}

TEST(VolumeSliderWheel, AccumulatesNotchesAndPresets) {
  EXPECT_EQ(30, VolumeSliderWheel::kRotationPerStep);
  const VolumeSliderWheel::Result none = VolumeSliderWheel::FromAngleDelta(0, 0);
  EXPECT_EQ(0, none.steps);
  const VolumeSliderWheel::Result up = VolumeSliderWheel::FromAngleDelta(0, 30);
  EXPECT_EQ(1, up.steps);
  const VolumeSliderWheel::Result down = VolumeSliderWheel::FromAngleDelta(0, -30);
  EXPECT_EQ(-1, down.steps);
  const VolumeSliderWheel::Result gtk_up = VolumeSliderWheel::FromGtkScroll(0, -1.0);
  EXPECT_EQ(1, gtk_up.steps);
  EXPECT_EQ(55u, VolumeSliderWheel::ApplySteps(50, 5));
  EXPECT_EQ(0u, VolumeSliderWheel::ApplySteps(2, -5));
  EXPECT_EQ(100u, VolumeSliderWheel::ApplySteps(98, 5));
  EXPECT_EQ("73%", VolumeSliderWheel::PercentLabel(73));
  const auto presets = VolumeSliderWheel::Presets();
  EXPECT_EQ(100, presets[0]);
  EXPECT_EQ(0, presets[5]);
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
  ASSERT_GE(rows.size(), 5u);
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

TEST(ContextAlbum, CoverPixelSizeFollowsPanelWidth) {
  EXPECT_EQ(ContextAlbum::kDefaultCoverSize, ContextAlbum::CoverPixelSize(0));
  EXPECT_EQ(ContextAlbum::kMinCoverSize, ContextAlbum::CoverPixelSize(20));
  EXPECT_EQ(184, ContextAlbum::CoverPixelSize(200));
  EXPECT_EQ(190, ContextAlbum::CoverPixelSize(200, 10));
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

TEST(ContextTechnical, ExtraMetadataRows) {
  Song song;
  song.set_valid(true);
  song.set_title("Roads");
  song.set_year(1994);
  song.set_genre("Trip hop");
  song.set_composer("Beth Gibbons");
  song.set_performer("Portishead");
  song.set_grouping("Dummy");
  song.set_comment("Remaster");
  song.set_disc(1);
  song.set_track(3);
  song.set_playcount(12);
  song.set_skipcount(2);
  song.set_rating(0.8f);
  song.set_basefilename("roads.flac");
  song.set_filesize(123456);
  const auto rows = ContextTechnical::Rows(song);
  auto value = [&](const char *key) {
    for (const auto &row : rows) {
      if (row.first == key) {
        return row.second;
      }
    }
    return std::string();
  };
  EXPECT_EQ("1994", value("Year"));
  EXPECT_EQ("Trip hop", value("Genre"));
  EXPECT_EQ("Beth Gibbons", value("Composer"));
  EXPECT_EQ("Portishead", value("Performer"));
  EXPECT_EQ("Dummy", value("Grouping"));
  EXPECT_EQ("Remaster", value("Comment"));
  EXPECT_EQ("1", value("Disc"));
  EXPECT_EQ("3", value("Track"));
  EXPECT_EQ("12", value("Play count"));
  EXPECT_EQ("2", value("Skip count"));
  EXPECT_EQ("4 / 5", value("Rating"));
  EXPECT_EQ("roads.flac", value("Filename"));
  EXPECT_FALSE(value("Filesize").empty());
}

TEST(DeleteFilesPolicy, SourceAndAllowFlags) {
  EXPECT_FALSE(DeleteFilesPolicy::Allowed(DeleteFilesPolicy::Source::Collection, false, true));
  EXPECT_TRUE(DeleteFilesPolicy::Allowed(DeleteFilesPolicy::Source::Collection, true, false));
  EXPECT_FALSE(DeleteFilesPolicy::Allowed(DeleteFilesPolicy::Source::Playlist, true, false));
  EXPECT_TRUE(DeleteFilesPolicy::Allowed(DeleteFilesPolicy::Source::Playlist, false, true));
  EXPECT_EQ(DeleteFilesPolicy::Source::Playlist, DeleteFilesPolicy::SourceForSongs({}));
  Song collection;
  collection.set_source(Song::Source::Collection);
  collection.set_url("file:///tmp/a.flac");
  EXPECT_EQ(DeleteFilesPolicy::Source::Collection, DeleteFilesPolicy::SourceForSongs({collection}));
  Song local;
  local.set_source(Song::Source::LocalFile);
  local.set_url("file:///tmp/b.flac");
  EXPECT_EQ(DeleteFilesPolicy::Source::Playlist, DeleteFilesPolicy::SourceForSongs({collection, local}));
}

TEST(ContextIdle, HeadlineScaleAndTotals) {
  EXPECT_STREQ("No song playing", ContextIdle::Headline());
  EXPECT_DOUBLE_EQ(1.6, ContextIdle::FontScale());
  EXPECT_EQ(18, ContextIdle::IdleFontSizePt(11));
  EXPECT_EQ(1, ContextIdle::IdleFontSizePt(0));
  EXPECT_EQ("1 song\n1 artist\n1 album", ContextIdle::TotalsMarkup(1, 1, 1));
}

TEST(ContextPlayingText, EscapesAndCombinesHeader) {
  EXPECT_EQ("&amp;&lt;&gt;", ContextPlayingText::EscapeMarkup("&<>"));
  EXPECT_EQ("<b>Roads</b>\nDummy", ContextPlayingText::TopMarkup("Roads", "Dummy"));
  EXPECT_EQ("<b>A &amp; B</b>", ContextPlayingText::TopMarkup("A & B", {}));
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

TEST(OrganizeFilename, ProblematicFatAsciiAndPrefixes) {
  EXPECT_EQ("ABCDEFGH", OrganizeFilename::RemoveProblematic("A:B?C*D\"E<F>G|H"));
  EXPECT_EQ("Mr Brightside", OrganizeFilename::RemoveDots("Mr. Brightside"));
  EXPECT_TRUE(OrganizeFilename::IsFatAllowed('A'));
  EXPECT_TRUE(OrganizeFilename::IsFatAllowed('/'));
  EXPECT_FALSE(OrganizeFilename::IsFatAllowed(':'));
  EXPECT_EQ("AB", OrganizeFilename::RemoveNonFat("A:B"));
  EXPECT_EQ("Artist/Album", OrganizeFilename::StripInvalidPrefixes("Artist/.Album"));
  EXPECT_EQ("hidden", OrganizeFilename::StripInvalidPrefixes(".hidden"));
  EXPECT_EQ("A B", OrganizeFilename::CollapseWhitespace("A   B"));
  EXPECT_EQ("A_B", OrganizeFilename::ReplaceSpaces("A B"));
  EXPECT_TRUE(OrganizeFilename::ShouldTransliterate({false, true, false, false, false}));
  EXPECT_TRUE(OrganizeFilename::ShouldTransliterate({false, false, true, false, false}));
  EXPECT_FALSE(OrganizeFilename::ShouldTransliterate({false, false, true, true, false}));

  OrganizeFilename::Options options;
  options.remove_problematic = true;
  options.replace_spaces = true;
  EXPECT_EQ("Track_Name", OrganizeFilename::Apply("Track: Name", options));
  options = {};
  options.remove_non_ascii = true;
  const std::string ascii = OrganizeFilename::Apply(StrUtils::Transliterate("Café"), options);
  EXPECT_EQ("Cafe", ascii);
}

TEST(OrganizeFormat, SanitizesFilenames) {
  OrganizeFormat format("%artist/%title");
  format.set_remove_problematic(true);
  Song song;
  song.set_artist("AC/DC");
  song.set_title("Hells Bells?");
  EXPECT_EQ("AC_DC/Hells Bells", format.GetFilenameForSong(song));

  song.set_title("Mr. Brightside");
  EXPECT_EQ("AC_DC/Mr Brightside", format.GetFilenameForSong(song));

  format.set_replace_spaces(true);
  EXPECT_EQ("AC_DC/Mr_Brightside", format.GetFilenameForSong(song));

  OrganizeFormat fat("%title");
  fat.set_remove_non_fat(true);
  Song fat_song;
  fat_song.set_title("Hello*World");
  EXPECT_EQ("HelloWorld", fat.GetFilenameForSong(fat_song));

  const OrganizeFilename::Options flags = format.FilenameOptions();
  EXPECT_TRUE(flags.remove_problematic);
  EXPECT_TRUE(flags.replace_spaces);
  EXPECT_FALSE(flags.remove_non_fat);
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

TEST(OrganizePreview, AfterCopyChoicesAndUniqueTags) {
  const auto choices = OrganizePreview::AfterCopyChoices();
  ASSERT_EQ(2u, choices.size());
  EXPECT_EQ("keep", choices[0].first);
  EXPECT_EQ("Keep the original files", choices[0].second);
  EXPECT_EQ("delete", choices[1].first);
  EXPECT_EQ("Delete the original files", choices[1].second);
  EXPECT_STREQ("keep", OrganizePreview::AfterCopyId(false));
  EXPECT_STREQ("delete", OrganizePreview::AfterCopyId(true));
  EXPECT_FALSE(OrganizePreview::DeleteOriginals("keep"));
  EXPECT_TRUE(OrganizePreview::DeleteOriginals("delete"));
  EXPECT_TRUE(OrganizeFormat::IsUniqueTag("%title"));
  EXPECT_TRUE(OrganizeFormat::IsUniqueTag("%track"));
  EXPECT_FALSE(OrganizeFormat::IsUniqueTag("%album"));
  EXPECT_STREQ("%albumartist/%album{ (Disc %disc)}/{%track - }{%albumartist - }%album{ (Disc %disc)} - %title.%extension",
               OrganizeSettings::kDefaultFormat);
}

TEST(OrganizeFormat, InsertTagsMatchQtOrder) {
  const auto tags = OrganizeFormat::InsertTags();
  ASSERT_EQ(19u, tags.size());
  EXPECT_STREQ("Album", tags.front().first);
  EXPECT_STREQ("album", tags.front().second);
  EXPECT_STREQ("Album artist", tags[1].first);
  EXPECT_STREQ("albumartist", tags[1].second);
  EXPECT_STREQ("Artist's initial", tags[3].first);
  EXPECT_STREQ("artistinitial", tags[3].second);
  EXPECT_STREQ("File extension", tags[9].first);
  EXPECT_STREQ("extension", tags[9].second);
  EXPECT_STREQ("Sample rate", tags[15].first);
  EXPECT_STREQ("Title", tags[16].first);
  EXPECT_STREQ("title", tags[16].second);
  EXPECT_STREQ("Year", tags.back().first);
  EXPECT_STREQ("year", tags.back().second);
  EXPECT_TRUE(std::none_of(tags.begin(), tags.end(), [](const auto &tag) { return std::string(tag.second) == "lyrics"; }));
}

TEST(OrganizeFormat, UniqueFilenameFromTitleAndTrack) {
  Song song;
  song.set_title("Roads");
  song.set_album("Dummy");
  song.set_track(3);
  EXPECT_TRUE(OrganizeFormat("%title").GetFilenameForSongResult(song).unique_filename);
  EXPECT_TRUE(OrganizeFormat("%track").GetFilenameForSongResult(song).unique_filename);
  EXPECT_FALSE(OrganizeFormat("%album").GetFilenameForSongResult(song).unique_filename);
  EXPECT_TRUE(OrganizeFormat("{%title}").GetFilenameForSongResult(song).unique_filename);
  Song untitled;
  untitled.set_album("Dummy");
  EXPECT_FALSE(OrganizeFormat("{%title}").GetFilenameForSongResult(untitled).unique_filename);
  EXPECT_FALSE(OrganizeFormat("%album/{%track - }%title").GetFilenameForSongResult(untitled).unique_filename);
}

TEST(OrganizePreview, DisambiguatesOnlyWhenUnique) {
  std::map<std::string, int> counts;
  EXPECT_EQ("foo.mp3", OrganizePreview::Disambiguate("foo.mp3", &counts));
  EXPECT_EQ("foo(2).mp3", OrganizePreview::Disambiguate("foo.mp3", &counts));
  EXPECT_EQ("foo(3).mp3", OrganizePreview::Disambiguate("foo.mp3", &counts));

  Song a;
  a.set_title("Same");
  a.set_album("Dummy");
  a.set_url("file:///tmp/a.flac");
  a.set_filesize(100);
  Song b;
  b.set_title("Same");
  b.set_album("Dummy");
  b.set_url("file:///tmp/b.flac");
  b.set_filesize(50);
  const auto unique = OrganizePreview::Compute({a, b}, OrganizeFormat("%title"));
  ASSERT_EQ(2u, unique.size());
  EXPECT_TRUE(unique[0].ok);
  EXPECT_TRUE(unique[1].ok);
  EXPECT_EQ("Same.flac", unique[0].relative_path);
  EXPECT_EQ("Same(2).flac", unique[1].relative_path);
  EXPECT_TRUE(OrganizePreview::CanProceed(unique));
  EXPECT_STREQ("dialog-ok-apply-symbolic", OrganizePreview::PreviewIconName(unique[0]));

  const auto album = OrganizePreview::Compute({a, b}, OrganizeFormat("%album"));
  ASSERT_EQ(2u, album.size());
  EXPECT_FALSE(album[0].ok);
  EXPECT_FALSE(album[1].ok);
  EXPECT_EQ(album[0].relative_path, album[1].relative_path);
  EXPECT_FALSE(OrganizePreview::CanProceed(album));
  EXPECT_STREQ("dialog-warning-symbolic", OrganizePreview::PreviewIconName(album[0]));

  Song empty;
  const auto missing = OrganizePreview::Compute({empty}, OrganizeFormat("%title"));
  ASSERT_EQ(1u, missing.size());
  EXPECT_TRUE(missing[0].relative_path.empty());
  EXPECT_TRUE(OrganizePreview::AnyEmptyPath(missing));
  EXPECT_FALSE(OrganizePreview::CanProceed(missing));
  EXPECT_EQ(150, OrganizePreview::TotalBytes({a, b}));
  EXPECT_TRUE(OrganizePreview::FitsOnDevice(50, 40, 100));
  EXPECT_FALSE(OrganizePreview::FitsOnDevice(70, 40, 100));
  EXPECT_TRUE(OrganizePreview::FitsOnDevice(999, 0, 0));
}

TEST(Organize, CopiesDisambiguatedCollisions) {
  char dir_template[] = "/tmp/strawberry-organize-dup-XXXXXX";
  const std::string dir = mkdtemp(dir_template);
  const std::string src_dir = FileUtils::Join(dir, "src");
  const std::string dest_dir = FileUtils::Join(dir, "dest");
  g_mkdir_with_parents(src_dir.c_str(), 0755);
  g_mkdir_with_parents(dest_dir.c_str(), 0755);
  const std::string src_a = FileUtils::Join(src_dir, "a.flac");
  const std::string src_b = FileUtils::Join(src_dir, "b.flac");
  ASSERT_TRUE(FileUtils::WriteFile(src_a, "audio-a"));
  ASSERT_TRUE(FileUtils::WriteFile(src_b, "audio-b"));

  Song a;
  a.set_valid(true);
  a.set_title("Same");
  a.set_url(FileUtils::UriFromPath(src_a));
  Song b;
  b.set_valid(true);
  b.set_title("Same");
  b.set_url(FileUtils::UriFromPath(src_b));

  const auto errors = Organize().Copy({a, b}, dest_dir, OrganizeFormat("%title"), Organize::Options{});
  EXPECT_TRUE(errors.empty());
  EXPECT_EQ("audio-a", FileUtils::ReadFile(FileUtils::Join(dest_dir, "Same.flac")));
  EXPECT_EQ("audio-b", FileUtils::ReadFile(FileUtils::Join(dest_dir, "Same(2).flac")));

  FileUtils::Remove(src_a);
  FileUtils::Remove(src_b);
  FileUtils::Remove(FileUtils::Join(dest_dir, "Same.flac"));
  FileUtils::Remove(FileUtils::Join(dest_dir, "Same(2).flac"));
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
  EXPECT_EQ("Rock", eq.selected_preset());
  EXPECT_EQ(-100, Equalizer::ClampBalance(-250));
  EXPECT_EQ(100, Equalizer::ClampBalance(250));
  EXPECT_EQ(0, Equalizer::ClampBalance(0));
}

TEST(EqualizerPresets, AfterDeleteAndNextSelected) {
  const std::vector<std::string> presets = {"Custom", "Rock", "User"};
  const auto remaining = EqualizerPresets::AfterDelete(presets, "User");
  ASSERT_EQ(2u, remaining.size());
  EXPECT_EQ("Custom", remaining[0]);
  EXPECT_EQ("Rock", remaining[1]);
  EXPECT_EQ("Rock", EqualizerPresets::NextSelected(remaining, "User", "Rock"));
  EXPECT_EQ("Custom", EqualizerPresets::NextSelected(remaining, "User", "User"));
  EXPECT_EQ(1, EqualizerPresets::IndexOf(remaining, "Rock"));
  EXPECT_EQ("Custom", EqualizerPresets::NextSelected({}, "User", "User"));
}

TEST(EqualizerPersist, SelectedPresetAndStereoBalancer) {
  EXPECT_EQ("Custom", EqualizerPersist::PresetOrDefault({}));
  EXPECT_EQ("Rock", EqualizerPersist::PresetOrDefault("Rock"));
  EXPECT_EQ(0, EqualizerPersist::EffectiveBalance(false, 80));
  EXPECT_EQ(80, EqualizerPersist::EffectiveBalance(true, 80));
  EXPECT_EQ(-100, EqualizerPersist::EffectiveBalance(true, -250));
  EXPECT_FLOAT_EQ(0.0f, EqualizerPersist::EffectiveBalanceFraction(false, 50));
  EXPECT_FLOAT_EQ(0.5f, EqualizerPersist::EffectiveBalanceFraction(true, 50));
  EXPECT_EQ(0, EqualizerPersist::EffectivePreamp(false, 6));
  EXPECT_EQ(6, EqualizerPersist::EffectivePreamp(true, 6));
  const std::vector<int> rock(10, 4);
  EXPECT_EQ(std::vector<int>(10, 0), EqualizerPersist::EffectiveGains(false, rock));
  EXPECT_EQ(rock, EqualizerPersist::EffectiveGains(true, rock));
  EXPECT_TRUE(EqualizerPersist::MigrateBalancerEnabled(true, true, 0));
  EXPECT_FALSE(EqualizerPersist::MigrateBalancerEnabled(true, false, 40));
  EXPECT_TRUE(EqualizerPersist::MigrateBalancerEnabled(false, false, 40));
  EXPECT_FALSE(EqualizerPersist::MigrateBalancerEnabled(false, false, 0));
  EXPECT_EQ("0 dB", EqualizerPersist::DbLabel(0));
  EXPECT_EQ("6 dB", EqualizerPersist::DbLabel(6));
  EXPECT_EQ("-12 dB", EqualizerPersist::DbLabel(-12));

  Equalizer eq;
  eq.set_stereo_balancer_enabled(true);
  eq.set_stereo_balance(250);
  EXPECT_TRUE(eq.stereo_balancer_enabled());
  EXPECT_EQ(100, eq.stereo_balance());
  EXPECT_FLOAT_EQ(1.0f, eq.EffectiveBalanceFraction());
  eq.set_stereo_balancer_enabled(false);
  EXPECT_FLOAT_EQ(0.0f, eq.EffectiveBalanceFraction());
  eq.set_enabled(false);
  eq.set_preamp(8);
  EXPECT_EQ("Custom", eq.selected_preset());
  EXPECT_EQ(0, eq.EffectivePreamp());
  EXPECT_EQ(std::vector<int>(10, 0), eq.EffectiveGains());
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
  TranscoderOptionsFields::Mp3 lame;
  lame.target = 0;
  lame.quality = 4;
  lame.cbr = true;
  const std::string vbr = lame.Pipeline();
  EXPECT_NE(std::string::npos, vbr.find("target=0"));
  EXPECT_NE(std::string::npos, vbr.find("quality=4"));
  EXPECT_NE(std::string::npos, vbr.find("cbr=true"));
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

TEST(OrganizeTranscode, CheckModesAndBestFormat) {
  EXPECT_EQ(Song::FileType::Unknown, OrganizeTranscode::Check(Song::FileType::FLAC, MusicStorage::TranscodeMode::Transcode_Never,
                                                              Song::FileType::MPEG, {Song::FileType::MPEG}));
  EXPECT_EQ(Song::FileType::MPEG, OrganizeTranscode::Check(Song::FileType::FLAC, MusicStorage::TranscodeMode::Transcode_Always,
                                                          Song::FileType::MPEG, {}));
  EXPECT_EQ(Song::FileType::Unknown, OrganizeTranscode::Check(Song::FileType::MPEG, MusicStorage::TranscodeMode::Transcode_Always,
                                                              Song::FileType::MPEG, {}));
  EXPECT_EQ(Song::FileType::Unknown, OrganizeTranscode::Check(Song::FileType::MPEG, MusicStorage::TranscodeMode::Transcode_Unsupported,
                                                              Song::FileType::MPEG, {Song::FileType::MPEG, Song::FileType::MP4}));
  EXPECT_EQ(Song::FileType::MPEG, OrganizeTranscode::Check(Song::FileType::FLAC, MusicStorage::TranscodeMode::Transcode_Unsupported,
                                                          Song::FileType::MPEG, {Song::FileType::MPEG, Song::FileType::MP4}));
  EXPECT_EQ(Song::FileType::Unknown, OrganizeTranscode::Check(Song::FileType::FLAC, MusicStorage::TranscodeMode::Transcode_Unsupported,
                                                              Song::FileType::MPEG, {}));
  EXPECT_EQ(Song::FileType::FLAC, OrganizeTranscode::PickBestFormat({Song::FileType::MPEG, Song::FileType::FLAC}));
  EXPECT_EQ(Song::FileType::MPEG, OrganizeTranscode::PickBestFormat({Song::FileType::MPEG, Song::FileType::MP4}));
  EXPECT_EQ(Song::FileType::Unknown, OrganizeTranscode::PickBestFormat({}));
  EXPECT_EQ(MusicStorage::TranscodeMode::Transcode_Never,
            OrganizeTranscode::FromDeviceMode(DeviceDatabaseBackend::TranscodeMode::Transcode_Never));
  EXPECT_EQ(MusicStorage::TranscodeMode::Transcode_Always,
            OrganizeTranscode::FromDeviceMode(DeviceDatabaseBackend::TranscodeMode::Transcode_Always));
  EXPECT_EQ(MusicStorage::TranscodeMode::Transcode_Unsupported,
            OrganizeTranscode::FromDeviceMode(DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported));
  EXPECT_EQ(Transcoder::Format::MP3, OrganizeTranscode::FormatFromFileType(Song::FileType::MPEG));
  EXPECT_EQ(Transcoder::Format::AAC, OrganizeTranscode::FormatFromFileType(Song::FileType::ALAC));
  EXPECT_EQ("mp3", OrganizeTranscode::ExtensionForFileType(Song::FileType::MPEG));
  EXPECT_EQ("/tmp/song.mp3", OrganizeTranscode::FiddleExtension("/tmp/song.flac", "mp3"));
  EXPECT_EQ("song.m4a", OrganizeTranscode::FiddleExtension("song", "m4a"));
  EXPECT_TRUE(OrganizeTranscode::SupportedForBackend("gio").empty());
  EXPECT_TRUE(OrganizeTranscode::Contains(OrganizeTranscode::SupportedForBackend("gpod"), Song::FileType::MPEG));
  EXPECT_TRUE(OrganizeTranscode::Contains(OrganizeTranscode::SupportedForBackend("mtp"), Song::FileType::ASF));
}

TEST(DeviceManager, MusicPathAndPassthroughPrepare) {
  ConnectedDevice usb;
  usb.backend = "gio";
  usb.mount_path = "/media/usb";
  EXPECT_EQ("/media/usb/Music", DeviceManager::MusicPath(usb));
  ConnectedDevice ipod;
  ipod.backend = "gpod";
  ipod.mount_path = "/media/ipod";
  EXPECT_EQ("/media/ipod", DeviceManager::MusicPath(ipod));
  EXPECT_TRUE(DeviceManager::MusicPath(ConnectedDevice{}).empty());

  DeviceManager manager;
  Song song;
  song.set_valid(true);
  song.set_title("Roads");
  song.set_filetype(Song::FileType::FLAC);
  song.set_url("file:///tmp/roads.flac");
  ConnectedDevice device;
  device.backend = "gio";
  device.unique_id = "test-usb";
  const SongList prepared = manager.TranscodeForDevice({song}, device);
  ASSERT_EQ(1u, prepared.size());
  EXPECT_EQ(song.url(), prepared[0].url());
  EXPECT_EQ(Song::FileType::FLAC, prepared[0].filetype());
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

TEST(MainWindowMenu, MatchesQtDailyActionLabels) {
  EXPECT_STREQ("Update changed collection folders", MainWindowMenu::UpdateCollection());
  EXPECT_STREQ("Edit track information...", MainWindowMenu::EditTrack());
  EXPECT_STREQ("Renumber tracks in this order...", MainWindowMenu::RenumberTracks());
  EXPECT_STREQ("Set value for all selected tracks...", MainWindowMenu::SetValue());
  EXPECT_STREQ("Love", MainWindowMenu::Love());
  EXPECT_STREQ("Toggle scrobbling", MainWindowMenu::ToggleScrobbling());
  EXPECT_STREQ("Shuffle mode", MainWindowMenu::ShuffleMode());
  EXPECT_STREQ("Repeat mode", MainWindowMenu::RepeatMode());
  EXPECT_STREQ("win.toggle-scrobbling", MainWindowMenu::ToggleScrobblingAction());
}

TEST(MainWindowLook, SidebarAndMuteMatchQt) {
  EXPECT_TRUE(MainWindowSettings::kDefaultShowSidebar);
  EXPECT_STREQ("MainWindow", MainWindowSettings::kSettingsGroup);
  EXPECT_STREQ("show_sidebar", MainWindowSettings::kShowSidebar);
  EXPECT_TRUE(MainWindowLook::ShowSidebar(true));
  EXPECT_FALSE(MainWindowLook::ShowSidebar(false));
  EXPECT_TRUE(MainWindowLook::DefaultShowSidebar());
  EXPECT_TRUE(MainWindowLook::MuteVisible(true));
  EXPECT_FALSE(MainWindowLook::MuteVisible(false));
  EXPECT_TRUE(MainWindowLook::MuteVisibleFromSettings(true));
  EXPECT_FALSE(MainWindowLook::MuteVisibleFromSettings(false));
  EXPECT_TRUE(MainWindowLook::IsMuted(0));
  EXPECT_FALSE(MainWindowLook::IsMuted(1));
  EXPECT_FALSE(MainWindowLook::IsMuted(50));
  EXPECT_STREQ("audio-volume-muted-symbolic", MainWindowLook::MuteIconName(true));
  EXPECT_STREQ("audio-volume-high-symbolic", MainWindowLook::MuteIconName(false));
  EXPECT_STREQ("Unmute", MainWindowLook::MuteTooltip(true));
  EXPECT_STREQ("Mute", MainWindowLook::MuteTooltip(false));
  EXPECT_STREQ("<Control>m", MainWindowLook::MuteAccel());
  EXPECT_STREQ("<Control>w", MainWindowLook::ClosePlaylistAccel());
  EXPECT_STREQ("<Control>d", MainWindowLook::PlaylistQueueAccel());
  EXPECT_STREQ("<Control><Shift>d", MainWindowLook::QueuePlayNextAccel());
}

TEST(MainWindowKeyboard, MatchesQtTransportKeys) {
  EXPECT_EQ(MainWindowKeyboard::Action::PlayPause, MainWindowKeyboard::FromWindowKey(MainWindowKeyboard::kSpace));
  EXPECT_EQ(MainWindowKeyboard::Action::PlayPause, MainWindowKeyboard::FromWindowKey(MainWindowKeyboard::kKPSpace));
  EXPECT_EQ(MainWindowKeyboard::Action::SeekBack, MainWindowKeyboard::FromWindowKey(MainWindowKeyboard::kLeft));
  EXPECT_EQ(MainWindowKeyboard::Action::SeekForward, MainWindowKeyboard::FromWindowKey(MainWindowKeyboard::kRight));
  EXPECT_EQ(MainWindowKeyboard::Action::None, MainWindowKeyboard::FromWindowKey('k'));
  EXPECT_FALSE(MainWindowKeyboard::ShouldHandleWindowKey(true));
  EXPECT_TRUE(MainWindowKeyboard::ShouldHandleWindowKey(false));
  EXPECT_EQ(MainWindowKeyboard::Action::Previous, MainWindowKeyboard::FromAccelKey(MainWindowKeyboard::kF5));
  EXPECT_EQ(MainWindowKeyboard::Action::PlayPause, MainWindowKeyboard::FromAccelKey(MainWindowKeyboard::kF6));
  EXPECT_EQ(MainWindowKeyboard::Action::Stop, MainWindowKeyboard::FromAccelKey(MainWindowKeyboard::kF7));
  EXPECT_EQ(MainWindowKeyboard::Action::Next, MainWindowKeyboard::FromAccelKey(MainWindowKeyboard::kF8));
  EXPECT_STREQ("F5", MainWindowKeyboard::PreviousAccel());
  EXPECT_STREQ("F6", MainWindowKeyboard::PlayPauseAccel());
  EXPECT_STREQ("F7", MainWindowKeyboard::StopAccel());
  EXPECT_STREQ("F8", MainWindowKeyboard::NextAccel());
  EXPECT_STREQ("F1", MainWindowKeyboard::AboutAccel());
  EXPECT_STREQ("<Control>l", MainWindowKeyboard::LoveAccel());
  EXPECT_STREQ("<Control><Alt>v", MainWindowKeyboard::StopAfterAccel());
  EXPECT_STREQ("<Control><Shift>h", MainWindowKeyboard::ShuffleAccel());
  EXPECT_STREQ("<Control><Shift>a", MainWindowKeyboard::AddFileAccel());
  EXPECT_STREQ("<Control>o", MainWindowKeyboard::OpenFilesAccel());
  EXPECT_STREQ("<Control>j", MainWindowKeyboard::JumpAccel());
  EXPECT_STREQ("<Control><Shift>o", MainWindowKeyboard::LoadPlaylistAccel());
  EXPECT_STREQ("<Control>t", MainWindowKeyboard::AutoCompleteTagsAccel());
  EXPECT_STREQ("<Control><Shift>t", MainWindowKeyboard::TranscodeSelectedAccel());
  EXPECT_STREQ("<Control>k", MainWindowKeyboard::ClearPlaylistAccel());
  EXPECT_STREQ("<Control>e", MainWindowKeyboard::EditTrackAccel());
  EXPECT_STREQ("<Control>p", MainWindowKeyboard::SettingsAccel());
  EXPECT_STREQ("<Control>comma", MainWindowKeyboard::SettingsCommaAccel());
}

TEST(FilterSearchKeyboard, MatchesCollectionFilterKeys) {
  EXPECT_EQ(FilterSearchKeyboard::Action::Activate, FilterSearchKeyboard::FromSearchKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(FilterSearchKeyboard::Action::MoveDown, FilterSearchKeyboard::FromSearchKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(FilterSearchKeyboard::Action::Clear, FilterSearchKeyboard::FromSearchKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(FilterSearchKeyboard::Action::FocusFilter, FilterSearchKeyboard::FromTreeKey(ListBoxKeyboard::kBackSpace));
  EXPECT_EQ(ListBoxKeyboard::Action::MoveUp, FilterSearchKeyboard::MoveAction(FilterSearchKeyboard::Action::MoveUp));
}

TEST(Appearance, BackgroundCssForTypesAndUrls) {
  EXPECT_EQ("file:///tmp/cover.jpg", Appearance::CssUrl("/tmp/cover.jpg"));
  EXPECT_EQ("https://example/a.png", Appearance::CssUrl("https://example/a.png"));
  EXPECT_EQ("top left", Appearance::BackgroundPositionCss(1));
  EXPECT_EQ("bottom right", Appearance::BackgroundPositionCss(5));
  EXPECT_TRUE(Appearance::BuildBackgroundCss(0, {}, 5, 0, 40).empty());
  const std::string none = Appearance::BuildBackgroundCss(1, {}, 5, 0, 40);
  EXPECT_NE(std::string::npos, none.find(Appearance::kMainSelector));
  EXPECT_NE(std::string::npos, none.find(Appearance::kPlaylistViewportSelector));
  EXPECT_NE(std::string::npos, none.find("background-image: none"));
  EXPECT_EQ(std::string::npos, none.find(std::string(Appearance::kMainSelector) + " { background-image: linear-gradient"));
  EXPECT_NE(std::string::npos, Appearance::BuildBackgroundCss(4, {}, 5, 0, 40).find("#8B1E3F"));
  const std::string custom = Appearance::BuildBackgroundCss(2, "/tmp/wall.jpg", 1, 8, 40);
  EXPECT_NE(std::string::npos, custom.find(Appearance::kPlaylistViewportSelector));
  EXPECT_NE(std::string::npos, custom.find("file:///tmp/wall.jpg"));
  EXPECT_NE(std::string::npos, custom.find("top left"));
  EXPECT_NE(std::string::npos, custom.find("blur(8px)"));
  EXPECT_NE(std::string::npos, custom.find("background-size: auto"));
  EXPECT_EQ(std::string::npos, custom.find(std::string(Appearance::kMainSelector) + " { background-image: linear-gradient"));
  const std::string stretched = Appearance::BuildBackgroundCss(2, "/tmp/wall.jpg", 1, 0, 40, true, true, false, 0);
  EXPECT_NE(std::string::npos, stretched.find("background-size: cover"));
  const std::string contained = Appearance::BuildBackgroundCss(2, "/tmp/wall.jpg", 1, 0, 40, true, true, true, 0);
  EXPECT_NE(std::string::npos, contained.find("background-size: contain"));
}

TEST(AppearanceColors, PaletteTabPlayingAndIconCss) {
  EXPECT_EQ("#353535", std::string(AppearanceColors::DarkHex(AppearanceSettings::kColorWindow)));
  EXPECT_TRUE(AppearanceColors::BuildPaletteCss(false, {{AppearanceSettings::kColorWindow, "#111111"}}).empty());
  const std::string palette = AppearanceColors::BuildPaletteCss(true, {{AppearanceSettings::kColorWindow, "#111111"},
                                                                      {AppearanceSettings::kColorWindowText, "#eeeeee"}});
  EXPECT_NE(std::string::npos, palette.find("background-color: #111111"));
  EXPECT_NE(std::string::npos, palette.find("color: #eeeeee"));
  EXPECT_TRUE(AppearanceColors::BuildTabBarCss(true, true, "#404040").empty());
  const std::string tab = AppearanceColors::BuildTabBarCss(false, true, "#404040");
  EXPECT_NE(std::string::npos, tab.find("background-color: #404040"));
  EXPECT_NE(std::string::npos, tab.find("linear-gradient"));
  EXPECT_EQ(".playlist-playing, .playlist-playing label { color: #6696e3; }", AppearanceColors::BuildPlayingSongCss("#6696e3"));
  EXPECT_EQ("auto", AppearanceColors::BackgroundSizeCss(false, true, true, 0));
  EXPECT_EQ("320px auto", AppearanceColors::BackgroundSizeCss(false, true, true, 320));
  EXPECT_EQ("contain", AppearanceColors::BackgroundSizeCss(true, true, true, 0));
  EXPECT_EQ("cover", AppearanceColors::BackgroundSizeCss(true, true, false, 0));
  EXPECT_EQ("100% 100%", AppearanceColors::BackgroundSizeCss(true, false, false, 0));
  AppearanceColors::IconSizes sizes;
  sizes.play_controls = 2;
  sizes.tabbar_small = 200;
  const AppearanceColors::IconSizes clamped = AppearanceColors::ClampIconSizes(sizes);
  EXPECT_EQ(AppearanceSettings::kDefaultIconSizePlayControlButtons, clamped.play_controls);
  EXPECT_EQ(128, clamped.tabbar_small);
  EXPECT_NE(std::string::npos, AppearanceColors::BuildIconSizeCss(sizes).find("32px"));
}

TEST(PlayingWidget, CoverSizeAndFade) {
  EXPECT_EQ(PlayingWidget::kSmallCover, PlayingWidget::CoverSize(PlayingWidget::Mode::SmallSongDetails, true, 400));
  EXPECT_EQ(PlayingWidget::kMaxCoverSize, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, false, 400));
  EXPECT_EQ(PlayingWidget::kLargeCover, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, false, 0));
  EXPECT_EQ(PlayingWidget::kMaxCoverSize, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, true, 800));
  EXPECT_EQ(80, PlayingWidget::CoverSize(PlayingWidget::Mode::LargeSongDetails, true, 20));
  EXPECT_EQ(0.0, PlayingWidget::FadeInOpacity(0));
  EXPECT_EQ(1.0, PlayingWidget::FadeInOpacity(PlayingWidget::kFadeTimelineMs));
  EXPECT_NEAR(0.5, PlayingWidget::FadeInOpacity(500), 0.001);
  EXPECT_NEAR(0.5, PlayingWidget::FadeOutOpacity(500), 0.001);
}

TEST(PlayingWidget, DetailsAndHeight) {
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  EXPECT_EQ("Roads", PlayingWidget::DetailsTitle(song));
  EXPECT_EQ("Portishead", PlayingWidget::DetailsArtist(song));
  EXPECT_EQ("Dummy", PlayingWidget::DetailsAlbum(song));
  EXPECT_TRUE(PlayingWidget::ShouldShow(true, true));
  EXPECT_FALSE(PlayingWidget::ShouldShow(true, false));
  EXPECT_EQ(4 + 260 + 40, PlayingWidget::LargeTotalHeight(260, 40));
  EXPECT_EQ(60, PlayingWidget::DetailsEstimate(true));
  EXPECT_EQ(40, PlayingWidget::DetailsEstimate(false));
  EXPECT_EQ(PlayingWidget::kSmallCover, PlayingWidget::TotalHeight(PlayingWidget::Mode::SmallSongDetails, 48, 20));
  EXPECT_EQ(4 + 260 + 60, PlayingWidget::TotalHeight(PlayingWidget::Mode::LargeSongDetails, 260, 60));
  EXPECT_EQ(0.0, PlayingWidget::ShowHideProgress(0));
  EXPECT_EQ(1.0, PlayingWidget::ShowHideProgress(PlayingWidget::kShowHideMs));
  EXPECT_EQ(130, PlayingWidget::AnimatedHeight(260, 250));
  EXPECT_EQ(50, PlayingWidget::ShowHideElapsed(true, 0, 50));
  EXPECT_EQ(450, PlayingWidget::ShowHideElapsed(false, 500, 50));
  EXPECT_EQ(PlayingWidget::kShowHideMs, PlayingWidget::ShowHideElapsed(true, 490, 50));
  EXPECT_TRUE(PlayingWidget::ShowHideFinished(true, PlayingWidget::kShowHideMs));
  EXPECT_TRUE(PlayingWidget::ShowHideFinished(false, 0));
  EXPECT_FALSE(PlayingWidget::ShowHideFinished(true, 250));
  EXPECT_TRUE(PlayingWidget::IsImagePath("cover.jpg"));
  EXPECT_FALSE(PlayingWidget::IsImagePath("track.mp3"));
}

TEST(ContextFormatTokens, InsertsKnownTokens) {
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%title%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%length%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%playcount%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%skipcount%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%rating%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%filename%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%url%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%originalyear%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%newline%"));
  EXPECT_TRUE(ContextFormatTokens::IsKnown("%grouping%"));
  EXPECT_FALSE(ContextFormatTokens::IsKnown("%unknown%"));
  EXPECT_EQ("%title%", ContextFormatTokens::Insert({}, "%title%"));
  EXPECT_EQ("%title%%artist%", ContextFormatTokens::Insert("%title%", "%artist%"));
  EXPECT_EQ("%title%", ContextFormatTokens::Insert("%title%", {}));
  EXPECT_EQ(19u, ContextFormatTokens::All().size());
  EXPECT_EQ("%artist%", ContextFormatTokens::All().front().first);
  EXPECT_EQ("%originalyear%", ContextFormatTokens::All().back().first);
}

TEST(ContextCover, ShouldSearch) {
  EXPECT_TRUE(ContextCover::ShouldSearch(true, true, false, false));
  EXPECT_FALSE(ContextCover::ShouldSearch(false, true, false, false));
  EXPECT_FALSE(ContextCover::ShouldSearch(true, false, false, false));
  EXPECT_FALSE(ContextCover::ShouldSearch(true, true, true, false));
  EXPECT_FALSE(ContextCover::ShouldSearch(true, true, false, true));
  EXPECT_EQ("Dummy", ContextCover::EffectiveAlbum("Dummy", "Roads"));
  EXPECT_EQ("Roads", ContextCover::EffectiveAlbum({}, "Roads"));
  EXPECT_TRUE(ContextCover::HasAlbumIdentity("Portishead", "Dummy"));
  EXPECT_FALSE(ContextCover::HasAlbumIdentity({}, "Dummy"));
  EXPECT_FALSE(ContextCover::HasAlbumIdentity("Portishead", {}));
  EXPECT_TRUE(ContextCover::HasExistingCover(true, false, false));
  EXPECT_TRUE(ContextCover::HasExistingCover(false, true, false));
  EXPECT_FALSE(ContextCover::HasExistingCover(false, false, false));
  EXPECT_TRUE(ContextCover::ArtPathLooksValid("file:///tmp/cover.jpg"));
  EXPECT_FALSE(ContextCover::ArtPathLooksValid({}));
  EXPECT_TRUE(ContextCover::ShouldSearchForSong(true, true, false, false, {}, {}, "Portishead", "Dummy"));
  EXPECT_FALSE(ContextCover::ShouldSearchForSong(true, true, false, true, {}, {}, "Portishead", "Dummy"));
  EXPECT_FALSE(ContextCover::ShouldSearchForSong(true, true, false, false, "/cover.jpg", {}, "Portishead", "Dummy"));
  EXPECT_FALSE(ContextCover::ShouldSearchForSong(true, true, false, false, {}, {}, {}, "Dummy"));
  EXPECT_FALSE(ContextCover::ShouldSearchForSong(false, true, false, false, {}, {}, "Portishead", "Dummy"));
}

TEST(ContextOptions, IdleMenuMatchesQt) {
  EXPECT_EQ(4, ContextOptions::ItemCount());
  EXPECT_EQ(ContextOptions::Action::ShowAlbum, ContextOptions::FromId("album"));
  EXPECT_EQ(ContextOptions::Action::ShowData, ContextOptions::FromId("data"));
  EXPECT_EQ(ContextOptions::Action::ShowLyrics, ContextOptions::FromId("lyrics"));
  EXPECT_EQ(ContextOptions::Action::SearchLyrics, ContextOptions::FromId("search-lyrics"));
  EXPECT_EQ(ContextOptions::Action::ShowAlbum, ContextOptions::FromId(nullptr));
  EXPECT_EQ("Show album cover", ContextOptions::Items()[0].label);
  EXPECT_EQ("Show song technical data", ContextOptions::Items()[1].label);
  EXPECT_EQ("Show song lyrics", ContextOptions::Items()[2].label);
  EXPECT_EQ("Automatically search for song lyrics", ContextOptions::Items()[3].label);
  EXPECT_TRUE(ContextOptions::IsIdle(false, false));
  EXPECT_FALSE(ContextOptions::IsIdle(true, false));
  EXPECT_FALSE(ContextOptions::IsIdle(false, true));
  EXPECT_TRUE(ContextOptions::ShowIdleMenu(true));
  EXPECT_FALSE(ContextOptions::ShowIdleMenu(false));
  EXPECT_TRUE(ContextOptions::ShowCoverMenu(false, true));
  EXPECT_FALSE(ContextOptions::ShowCoverMenu(true, true));
  EXPECT_FALSE(ContextOptions::ShowCoverMenu(false, false));
  EXPECT_TRUE(ContextOptions::TriggersLyricsSearch(ContextOptions::Action::ShowLyrics));
  EXPECT_TRUE(ContextOptions::TriggersLyricsSearch(ContextOptions::Action::SearchLyrics));
  EXPECT_FALSE(ContextOptions::TriggersLyricsSearch(ContextOptions::Action::ShowAlbum));
  EXPECT_TRUE(ContextOptions::Checked(ContextOptions::Action::ShowAlbum, true, false, true, true));
  EXPECT_FALSE(ContextOptions::Checked(ContextOptions::Action::ShowData, true, false, true, true));
  EXPECT_TRUE(ContextOptions::Toggle(ContextOptions::Action::ShowData, true, false, true, true));
  EXPECT_FALSE(ContextOptions::Toggle(ContextOptions::Action::SearchLyrics, true, false, true, true));
}

TEST(ContextFont, UsesFallbackSizeForFamilyOnly) {
  const FontUtils::Font family_only = ContextFont::Load("Noto Sans", 11);
  EXPECT_EQ("Noto Sans", family_only.family);
  EXPECT_EQ(11, family_only.size_pt);
  const FontUtils::Font pango = ContextFont::Load("Cantarell Bold 14", 10);
  EXPECT_EQ("Cantarell", pango.family);
  EXPECT_EQ(14, pango.size_pt);
  EXPECT_TRUE(pango.bold);
  EXPECT_EQ("#context-headline { font: 11pt \"Noto Sans\"; }", ContextFont::CssRule("#context-headline", family_only));
}

TEST(PlaylistLook, CombinedCssCoversAlternatingGlowAndBars) {
  EXPECT_TRUE(PlaylistLook::AlternatingCss(false).empty());
  EXPECT_NE(std::string::npos, PlaylistLook::AlternatingCss(true).find("playlist-alt"));
  EXPECT_NE(std::string::npos, PlaylistLook::GlowCss(true).find("playlist-glow"));
  EXPECT_NE(std::string::npos, PlaylistLook::BarsCss(true, 0.4).find("40%"));
  EXPECT_NE(std::string::npos, PlaylistLook::UnavailableCss().find("playlist-unavailable"));
  const std::string css = PlaylistLook::CombinedCss(true, true, true, 0.25);
  EXPECT_NE(std::string::npos, css.find("playlist-alt"));
  EXPECT_NE(std::string::npos, css.find("playlist-glow"));
  EXPECT_NE(std::string::npos, css.find("25%"));
  EXPECT_NE(std::string::npos, css.find("playlist-unavailable"));
}

TEST(PlaylistLook, GlowPulseMatchesQtSteps) {
  EXPECT_EQ(24, PlaylistLook::kGlowIntensitySteps);
  EXPECT_EQ(48, PlaylistLook::GlowPeriod());
  EXPECT_EQ(62, PlaylistLook::GlowIntervalMs());
  EXPECT_EQ(0, PlaylistLook::GlowFrame(0));
  EXPECT_EQ(23, PlaylistLook::GlowFrame(23));
  EXPECT_EQ(23, PlaylistLook::GlowFrame(24));
  EXPECT_EQ(22, PlaylistLook::GlowFrame(25));
  EXPECT_EQ(0, PlaylistLook::GlowFrame(47));
  EXPECT_EQ(1, PlaylistLook::NextGlowStep(0));
  EXPECT_EQ(0, PlaylistLook::NextGlowStep(47));
  EXPECT_EQ(24, PlaylistLook::StopGlowStep());
  EXPECT_NEAR(0.4, PlaylistLook::GlowOverlay(0), 0.0001);
  EXPECT_LT(PlaylistLook::GlowOverlay(23), PlaylistLook::GlowOverlay(0));
  EXPECT_TRUE(PlaylistLook::ShouldAnimateGlow(true, true, true));
  EXPECT_FALSE(PlaylistLook::ShouldAnimateGlow(true, false, true));
  EXPECT_FALSE(PlaylistLook::ShouldAnimateGlow(true, true, false));
  const std::string dim = PlaylistLook::GlowCss(true, 0);
  const std::string bright = PlaylistLook::GlowCss(true, 23);
  EXPECT_NE(dim, bright);
}

TEST(PlaylistBehaviour, CloseErrorGreyoutAndSort) {
  EXPECT_TRUE(PlaylistBehaviour::UrlsMatch("file:///tmp/a.flac", "/tmp/a.flac"));
  EXPECT_TRUE(PlaylistBehaviour::IsLocalUrl("file:///tmp/a.flac"));
  EXPECT_FALSE(PlaylistBehaviour::IsLocalUrl("http://example.invalid/live"));
  Song local;
  local.set_url("file:///tmp/a.flac");
  local.set_source(Song::Source::LocalFile);
  EXPECT_TRUE(PlaylistBehaviour::IsLocalMedia(local));
  Song stream;
  stream.set_url("http://example.invalid/live");
  stream.set_source(Song::Source::Stream);
  EXPECT_FALSE(PlaylistBehaviour::IsLocalMedia(stream));
  EXPECT_TRUE(PlaylistBehaviour::ApplyValidity(&local, false));
  EXPECT_TRUE(local.unavailable());
  EXPECT_TRUE(PlaylistBehaviour::ShouldGreyout(local));
  EXPECT_FALSE(PlaylistBehaviour::ApplyValidity(&local, false));
  EXPECT_TRUE(PlaylistBehaviour::ShouldPromptClose(true, false, false));
  EXPECT_FALSE(PlaylistBehaviour::ShouldPromptClose(true, true, false));
  EXPECT_FALSE(PlaylistBehaviour::ShouldPromptClose(true, false, true));
  EXPECT_FALSE(PlaylistBehaviour::ShouldPromptClose(false, false, false));
  EXPECT_TRUE(PlaylistBehaviour::ShouldStopAfterError(false, 1, 10));
  EXPECT_FALSE(PlaylistBehaviour::ShouldStopAfterError(true, 1, 10));
  EXPECT_TRUE(PlaylistBehaviour::ShouldStopAfterError(true, 10, 10));
  EXPECT_TRUE(PlaylistBehaviour::ColumnIsNumeric(PlaylistColumn::Year));
  EXPECT_FALSE(PlaylistBehaviour::ColumnIsNumeric(PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistBehaviour::LessThanText("A", "B", false, false));
  EXPECT_TRUE(PlaylistBehaviour::LessThanText("10", "2", true, true));
}

TEST(AnalyzerFramerate, QtDiscretePresets) {
  const auto presets = AnalyzerFramerate::Presets();
  ASSERT_EQ(4u, presets.size());
  EXPECT_EQ(AnalyzerSettings::kLowFramerate, presets[0].fps);
  EXPECT_EQ(AnalyzerSettings::kMediumFramerate, presets[1].fps);
  EXPECT_EQ(AnalyzerSettings::kHighFramerate, presets[2].fps);
  EXPECT_EQ(AnalyzerSettings::kSuperHighFramerate, presets[3].fps);
  EXPECT_EQ(20, AnalyzerSettings::kLowFramerate);
  EXPECT_EQ(25, AnalyzerSettings::kMediumFramerate);
  EXPECT_EQ(30, AnalyzerSettings::kHighFramerate);
  EXPECT_EQ(60, AnalyzerSettings::kSuperHighFramerate);
  EXPECT_EQ("Medium (25 fps)", AnalyzerFramerate::LabelFor(25));
  EXPECT_EQ(25, AnalyzerFramerate::Nearest(24));
  EXPECT_EQ(60, AnalyzerFramerate::Nearest(55));
}

TEST(TaskbarProgressHelpers, FractionAndVisibility) {
  EXPECT_DOUBLE_EQ(0.0, TaskbarProgressHelpers::Fraction(0, 0));
  EXPECT_DOUBLE_EQ(0.0, TaskbarProgressHelpers::Fraction(10, -1));
  EXPECT_DOUBLE_EQ(0.25, TaskbarProgressHelpers::Fraction(25, 100));
  EXPECT_DOUBLE_EQ(1.0, TaskbarProgressHelpers::Fraction(200, 100));
  EXPECT_DOUBLE_EQ(0.0, TaskbarProgressHelpers::Fraction(-5, 100));
  EXPECT_TRUE(TaskbarProgressHelpers::ShouldShow(true, true, 100));
  EXPECT_FALSE(TaskbarProgressHelpers::ShouldShow(false, true, 100));
  EXPECT_FALSE(TaskbarProgressHelpers::ShouldShow(true, false, 100));
  EXPECT_FALSE(TaskbarProgressHelpers::ShouldShow(true, true, 0));
  EXPECT_STREQ("application://org.strawberrymusicplayer.strawberry.desktop", TaskbarProgressHelpers::AppUri());
  EXPECT_STREQ("com.canonical.Unity.LauncherEntry", TaskbarProgressHelpers::Interface());
  EXPECT_EQ("/com/canonical/unity/launcherentry/strawberry", TaskbarProgressHelpers::ObjectPath());
}

TEST(MultiLoadingText, FormatsQtStatusBarCopy) {
  EXPECT_TRUE(MultiLoadingText::Format({}).empty());
  EXPECT_FALSE(MultiLoadingText::ShowIndicator(0));
  EXPECT_TRUE(MultiLoadingText::ShowIndicator(1));
  EXPECT_FALSE(StatusBarStack::ShowLoading(0));
  EXPECT_TRUE(StatusBarStack::ShowLoading(2));
  EXPECT_EQ(StatusBarStack::Page::Summary, StatusBarStack::PageForTaskCount(0));
  EXPECT_EQ(StatusBarStack::Page::Loading, StatusBarStack::PageForTaskCount(3));
  EXPECT_STREQ("summary", StatusBarStack::ChildName(StatusBarStack::Page::Summary));
  EXPECT_STREQ("loading", StatusBarStack::ChildName(StatusBarStack::Page::Loading));
  EXPECT_EQ("Scanning collection...", MultiLoadingText::Format({{"Scanning collection", 0, 0}}));
  EXPECT_EQ("Updating 40%...", MultiLoadingText::Format({{"Updating", 2, 5}}));
  EXPECT_EQ("Loading tags, fetching covers 50%...",
            MultiLoadingText::Format({{"Loading tags", 0, 0}, {"Fetching covers", 1, 2}}));
  EXPECT_EQ(40, MultiLoadingText::Percent(2, 5));
  EXPECT_EQ(0, MultiLoadingText::Percent(3, 0));
}

TEST(CollectionSettings, CacheSizeUnitsMatchQt) {
  EXPECT_EQ(0, static_cast<int>(CollectionSettings::CacheSizeUnit::KB));
  EXPECT_EQ(1, static_cast<int>(CollectionSettings::CacheSizeUnit::MB));
  EXPECT_EQ(2, static_cast<int>(CollectionSettings::CacheSizeUnit::GB));
  EXPECT_EQ(3, static_cast<int>(CollectionSettings::CacheSizeUnit::TB));
  EXPECT_STREQ("cache_size_unit", CollectionSettings::kSettingsCacheSizeUnit);
  EXPECT_STREQ("disk_cache_size_unit", CollectionSettings::kSettingsDiskCacheSizeUnit);
}

TEST(AddStreamUrl, ValidatesSchemeAndHostLikeQt) {
  EXPECT_STREQ("Add Stream", AddStreamUrl::Title());
  EXPECT_STREQ("Enter the URL of a stream:", AddStreamUrl::Prompt());
  EXPECT_TRUE(AddStreamUrl::IsValid("https://example.com/stream.mp3"));
  EXPECT_TRUE(AddStreamUrl::IsValid("http://host:8000/"));
  EXPECT_TRUE(AddStreamUrl::IsValid("https://[2001:db8::1]/live"));
  EXPECT_FALSE(AddStreamUrl::IsValid(""));
  EXPECT_FALSE(AddStreamUrl::IsValid("example.com"));
  EXPECT_FALSE(AddStreamUrl::IsValid("https://"));
  EXPECT_FALSE(AddStreamUrl::IsValid("://missing-scheme.example"));
  EXPECT_EQ("https", AddStreamUrl::Scheme("https://radio.example/live"));
  EXPECT_EQ("radio.example", AddStreamUrl::Host("https://radio.example/live"));
}

TEST(UserPassLabels, MatchQtDialogCopy) {
  EXPECT_STREQ("Enter username and password", UserPassLabels::Prompt());
  EXPECT_STREQ("Username", UserPassLabels::Username());
  EXPECT_STREQ("Password", UserPassLabels::Password());
}
