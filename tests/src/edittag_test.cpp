#include "constants/edittagdialogsettings.h"
#include "covermanager/coveroptions.h"
#include "dialogs/dialoglistkeyboard.h"
#include "dialogs/edittagcompleter.h"
#include "dialogs/edittagcover.h"
#include "dialogs/edittagsummaryfields.h"
#include "dialogs/edittagsummarylabels.h"
#include "dialogs/edittagcoverdrop.h"
#include "dialogs/edittagfieldreset.h"
#include "dialogs/edittagfields.h"
#include "dialogs/edittagid3v2.h"
#include "dialogs/edittagloading.h"
#include "dialogs/edittagsave.h"
#include "dialogs/edittagtabs.h"
#include "tagreader/tagreaderresult.h"
#include "utilities/fileutils.h"

#include <glib/gstdio.h>
#include <unistd.h>
#include "lyrics/lyricsproviderorder.h"
#include "lyrics/lyricsprovidersettings.h"
#include "tagreader/tagwriterfields.h"
#include "widgets/listboxkeyboard.h"

#include <gtest/gtest.h>

namespace {

Song MakeTagged(const std::string &title, const std::string &artist, const std::string &album) {
  Song song(Song::Source::Collection);
  song.set_title(title);
  song.set_artist(artist);
  song.set_album(album);
  song.set_albumartist(artist);
  song.set_year(1994);
  song.set_track(8);
  song.set_playcount(4);
  song.set_skipcount(2);
  song.set_lastplayed(1700000000);
  song.set_url("file:///tmp/music/" + title + ".flac");
  song.set_filetype(Song::FileType::FLAC);
  song.set_valid(true);
  return song;
}

}  // namespace

TEST(EditTagFields, CommonValueDetectsMixedAndShared) {
  Song roads = MakeTagged("Roads", "Portishead", "Dummy");
  Song glory = MakeTagged("Glory Box", "Portishead", "Dummy");
  glory.set_year(1994);
  const auto artist = EditTagFields::CommonValue({roads, glory}, [](const Song &song) { return song.artist(); });
  EXPECT_EQ("Portishead", artist.first);
  EXPECT_FALSE(artist.second);
  const auto title = EditTagFields::CommonValue({roads, glory}, [](const Song &song) { return song.title(); });
  EXPECT_TRUE(title.first.empty());
  EXPECT_TRUE(title.second);
  const auto empty = EditTagFields::CommonValue({}, [](const Song &song) { return song.title(); });
  EXPECT_TRUE(empty.first.empty());
  EXPECT_FALSE(empty.second);
}

TEST(EditTagFields, ApplyFieldWritesTextAndNumericTags) {
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  EditTagFields::ApplyField(&song, "Title", "Wandering Star");
  EditTagFields::ApplyField(&song, "Artist", "Portishead");
  EditTagFields::ApplyField(&song, "Album artist", "Portishead");
  EditTagFields::ApplyField(&song, "Year", "1994");
  EditTagFields::ApplyField(&song, "Track", "");
  EditTagFields::ApplyField(&song, "BPM", "120.5");
  EditTagFields::ApplyField(&song, "Comment", "remaster");
  EXPECT_EQ("Wandering Star", song.title());
  EXPECT_EQ("Portishead", song.albumartist());
  EXPECT_EQ(1994, song.year());
  EXPECT_EQ(-1, song.track());
  EXPECT_NEAR(120.5f, song.bpm(), 0.01f);
  EXPECT_EQ("remaster", song.comment());
}

TEST(EditTagFields, ApplyChangedFieldsUpdatesAllSongs) {
  SongList songs = {MakeTagged("Roads", "Portishead", "Dummy"), MakeTagged("Glory Box", "Portishead", "Dummy")};
  EditTagFields::ApplyChangedFields(&songs, {{"Genre", "Trip Hop"}, {"Year", "1994"}});
  EXPECT_EQ("Trip Hop", songs[0].genre());
  EXPECT_EQ("Trip Hop", songs[1].genre());
  EXPECT_EQ(1994, songs[1].year());
}

TEST(EditTagFieldReset, ShowsOnlyWhenValueChanged) {
  EXPECT_FALSE(EditTagFieldReset::ShouldShowReset("Roads", "Roads"));
  EXPECT_TRUE(EditTagFieldReset::ShouldShowReset("Glory Box", "Roads"));
  EXPECT_FALSE(EditTagFieldReset::ShouldShowReset(4.0, 4.0));
  EXPECT_TRUE(EditTagFieldReset::ShouldShowReset(2.5, 4.0));
  EXPECT_EQ("Roads", EditTagFieldReset::ResetValue("Roads"));
  EXPECT_STREQ("Reset this field", EditTagFieldReset::ResetTooltip());
}

TEST(EditTagFields, ResetPlayStatisticsClearsCounts) {
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  EditTagFields::ResetPlayStatistics(&song);
  EXPECT_EQ(0u, song.playcount());
  EXPECT_EQ(0u, song.skipcount());
  EXPECT_EQ(-1, song.lastplayed());
  SongList songs = {MakeTagged("A", "B", "C"), MakeTagged("D", "E", "F")};
  EditTagFields::ResetPlayStatistics(&songs);
  EXPECT_EQ(0u, songs[1].playcount());
  EXPECT_EQ(-1, songs[1].lastplayed());
}

TEST(EditTagFields, WrapIndexAndSongRowLabel) {
  EXPECT_EQ(0, EditTagFields::WrapIndex(0, 1, 0));
  EXPECT_EQ(0, EditTagFields::WrapIndex(2, 1, 3));
  EXPECT_EQ(2, EditTagFields::WrapIndex(0, -1, 3));
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  EXPECT_EQ("Portishead - Roads", EditTagFields::SongRowLabel(song));
}

TEST(EditTagTabs, ClampNameAndSettingsKeys) {
  EXPECT_STREQ("EditTagDialog", EditTagDialogSettings::kSettingsGroup);
  EXPECT_STREQ("current_tab", EditTagDialogSettings::kCurrentTab);
  EXPECT_EQ(0, EditTagTabs::ClampIndex(-2));
  EXPECT_EQ(3, EditTagTabs::ClampIndex(99));
  EXPECT_STREQ("Lyrics", EditTagTabs::Name(2));
  EXPECT_EQ(1, EditTagTabs::IndexFromName("Tags"));
  EXPECT_EQ(0, EditTagTabs::IndexFromName("Missing"));
}

TEST(EditTagId3v2, VersionForSongsAndCombo) {
  EXPECT_EQ(TagID3v2Version::V3, EditTagId3v2::TagVersionFromIndex(EditTagId3v2::kComboIndex3));
  EXPECT_EQ(TagID3v2Version::V4, EditTagId3v2::TagVersionFromIndex(EditTagId3v2::kComboIndex4));
  EXPECT_EQ(EditTagId3v2::kComboIndex3, EditTagId3v2::ComboIndex(3));
  EXPECT_EQ(EditTagId3v2::kComboIndex4, EditTagId3v2::ComboIndex(4));

  Song mpeg;
  mpeg.set_filetype(Song::FileType::MPEG);
  mpeg.set_id3v2_version(3);
  Song wav;
  wav.set_filetype(Song::FileType::WAV);
  wav.set_id3v2_version(3);
  Song flac;
  flac.set_filetype(Song::FileType::FLAC);
  EXPECT_TRUE(mpeg.id3v2_tags_supported());
  EXPECT_FALSE(flac.id3v2_tags_supported());
  EXPECT_TRUE(EditTagId3v2::AnySupported({mpeg, flac}));
  EXPECT_FALSE(EditTagId3v2::AnySupported({flac}));
  EXPECT_EQ(3, EditTagId3v2::VersionForSongs({mpeg, wav}));
  wav.set_id3v2_version(4);
  EXPECT_EQ(4, EditTagId3v2::VersionForSongs({mpeg, wav}));
  Song unknown;
  unknown.set_filetype(Song::FileType::MPEG);
  EXPECT_EQ(4, EditTagId3v2::VersionForSongs({unknown}));
}

TEST(EditTagCover, EmbeddedDefaultAndSaveType) {
  EXPECT_TRUE(Song::save_embedded_cover_supported(Song::FileType::FLAC));
  EXPECT_TRUE(Song::save_embedded_cover_supported(Song::FileType::MPEG));
  EXPECT_TRUE(Song::save_embedded_cover_supported(Song::FileType::OggOpus));
  EXPECT_FALSE(Song::save_embedded_cover_supported(Song::FileType::ASF));
  Song flac;
  flac.set_source(Song::Source::Collection);
  flac.set_filetype(Song::FileType::FLAC);
  flac.set_url("file:///tmp/dummy.flac");
  EXPECT_TRUE(flac.save_embedded_cover_supported());
  EXPECT_FALSE(EditTagCover::DefaultEmbeddedChecked(flac, CoverOptions::CoverType::Album));
  flac.set_art_embedded(true);
  EXPECT_TRUE(EditTagCover::DefaultEmbeddedChecked(flac, CoverOptions::CoverType::Album));
  Song mp3;
  mp3.set_source(Song::Source::Collection);
  mp3.set_filetype(Song::FileType::MPEG);
  mp3.set_url("file:///tmp/dummy.mp3");
  EXPECT_TRUE(EditTagCover::DefaultEmbeddedChecked(mp3, CoverOptions::CoverType::Embedded));
  EXPECT_EQ(CoverOptions::CoverType::Embedded, EditTagCover::EffectiveSaveType(CoverOptions::CoverType::Album, true));
  EXPECT_EQ(CoverOptions::CoverType::Album, EditTagCover::EffectiveSaveType(CoverOptions::CoverType::Album, false));
  EXPECT_TRUE(EditTagCover::AnySupported({flac}));
}

TEST(CoverOptions, TypeFilenameAndPattern) {
  EXPECT_EQ(CoverOptions::CoverType::Cache, CoverOptions::TypeFromValue("1"));
  EXPECT_EQ(CoverOptions::CoverType::Album, CoverOptions::TypeFromValue("2"));
  EXPECT_EQ(CoverOptions::CoverType::Embedded, CoverOptions::TypeFromValue("3"));
  EXPECT_EQ(CoverOptions::CoverType::Cache, CoverOptions::TypeFromValue("cache"));
  EXPECT_EQ(CoverOptions::CoverType::Album, CoverOptions::TypeFromValue("album"));
  EXPECT_EQ(CoverOptions::CoverFilename::Hash, CoverOptions::FilenameModeFromValue("1"));
  EXPECT_EQ(CoverOptions::CoverFilename::Pattern, CoverOptions::FilenameModeFromValue("2"));
  EXPECT_EQ(CoverOptions::CoverFilename::Pattern, CoverOptions::FilenameModeFromValue("pattern"));

  Song song;
  song.set_source(Song::Source::Collection);
  song.set_artist("Portishead");
  song.set_albumartist("Portishead");
  song.set_album("Dummy (Disc 1)");
  song.set_filetype(Song::FileType::ASF);
  CoverOptions options;
  options.cover_type = CoverOptions::CoverType::Embedded;
  EXPECT_EQ(CoverOptions::CoverType::Cache, options.EffectiveType(song));
  options.cover_type = CoverOptions::CoverType::Album;
  options.cover_filename = CoverOptions::CoverFilename::Pattern;
  options.cover_pattern = "%albumartist-%album";
  options.cover_lowercase = true;
  options.cover_replace_spaces = true;
  EXPECT_EQ("portishead-dummy.jpg", options.FilenameForSong(song));
}

TEST(EditTagCoverDrop, AcceptsImageUrisAndSkipsOtherFiles) {
  EXPECT_TRUE(EditTagCoverDrop::CanAcceptPath("/covers/dummy.jpg"));
  EXPECT_TRUE(EditTagCoverDrop::CanAcceptPath("cover.WEBP"));
  EXPECT_FALSE(EditTagCoverDrop::CanAcceptPath("/music/roads.flac"));
  EXPECT_EQ("/covers/a.png", EditTagCoverDrop::FirstImagePath("file:///covers/a.png\nfile:///music/roads.flac"));
  EXPECT_EQ("/covers/b.jpeg", EditTagCoverDrop::FirstImagePath("file://localhost/covers/b.jpeg"));
  EXPECT_TRUE(EditTagCoverDrop::FirstImagePath("file:///music/roads.flac").empty());
}

TEST(DialogListKeyboard, ActivateMoveAndWrap) {
  EXPECT_TRUE(DialogListKeyboard::IsActivate(ListBoxKeyboard::kReturn));
  EXPECT_TRUE(DialogListKeyboard::IsActivate(ListBoxKeyboard::kKPEnter));
  EXPECT_FALSE(DialogListKeyboard::IsActivate(ListBoxKeyboard::kDown));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kUp));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kDown));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kHome));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kEnd));
  EXPECT_FALSE(DialogListKeyboard::IsMove(ListBoxKeyboard::kReturn));
  EXPECT_FALSE(DialogListKeyboard::IsMove(ListBoxKeyboard::kDelete));
  EXPECT_EQ(2, DialogListKeyboard::NextIndex(0, 3, ListBoxKeyboard::kUp));
  EXPECT_EQ(0, DialogListKeyboard::NextIndex(2, 3, ListBoxKeyboard::kDown));
  EXPECT_EQ(0, DialogListKeyboard::NextIndex(2, 3, ListBoxKeyboard::kHome));
  EXPECT_EQ(2, DialogListKeyboard::NextIndex(0, 3, ListBoxKeyboard::kEnd));
  EXPECT_EQ(-1, DialogListKeyboard::NextIndex(0, 0, ListBoxKeyboard::kDown));
}

TEST(LyricsProviderOrder, ParseJoinMoveAndRank) {
  const std::vector<std::string> parsed = LyricsProviderOrder::Parse(" LrcLib , Genius, ,OVH ");
  ASSERT_EQ(3u, parsed.size());
  EXPECT_EQ("LrcLib", parsed[0]);
  EXPECT_EQ("Genius", parsed[1]);
  EXPECT_EQ("OVH", parsed[2]);
  EXPECT_EQ("LrcLib,Genius,OVH", LyricsProviderOrder::Join(parsed));
  EXPECT_EQ(1, LyricsProviderOrder::Rank(parsed, "Genius", 99));
  EXPECT_EQ(99, LyricsProviderOrder::Rank(parsed, "Missing", 99));
  const std::vector<std::string> moved = LyricsProviderOrder::Move(parsed, 0, 1);
  ASSERT_EQ(3u, moved.size());
  EXPECT_EQ("Genius", moved[0]);
  EXPECT_EQ("LrcLib", moved[1]);
  EXPECT_EQ(parsed, LyricsProviderOrder::Move(parsed, 0, -1));
}

TEST(LyricsProviderSettings, EnabledListAndStoredSemantics) {
  EXPECT_STREQ("Lyrics providers", LyricsProviderSettings::ProvidersGroup());
  EXPECT_STREQ("Choose the providers you want to use when searching for lyrics.", LyricsProviderSettings::ProvidersHint());
  EXPECT_STREQ("Move up", LyricsProviderSettings::MoveUp());
  EXPECT_STREQ("Move down", LyricsProviderSettings::MoveDown());
  EXPECT_FALSE(LyricsProviderSettings::EnabledFromStored(true, false, true, true));
  EXPECT_TRUE(LyricsProviderSettings::EnabledFromStored(true, true, true, false));
  EXPECT_TRUE(LyricsProviderSettings::EnabledFromStored(false, true, true, true));
  EXPECT_FALSE(LyricsProviderSettings::EnabledFromStored(false, true, true, false));
  EXPECT_TRUE(LyricsProviderSettings::EnabledFromStored(false, true, false, false));
  const std::vector<std::string> enabled =
      LyricsProviderSettings::EnabledNames({{"LrcLib", true}, {"Genius", false}, {"OVH", true}});
  ASSERT_EQ(2u, enabled.size());
  EXPECT_EQ("LrcLib", enabled.front());
  EXPECT_EQ("OVH", enabled.back());
}

TEST(EditTagCompleter, CompletesQtFieldsAndSuggestsPrefixes) {
  EXPECT_TRUE(EditTagCompleter::CompletesField("Artist"));
  EXPECT_TRUE(EditTagCompleter::CompletesField("Title sort"));
  EXPECT_TRUE(EditTagCompleter::CompletesField("Composer sort"));
  EXPECT_TRUE(EditTagCompleter::CompletesField("Performer sort"));
  EXPECT_FALSE(EditTagCompleter::CompletesField("Year"));
  EXPECT_FALSE(EditTagCompleter::CompletesField("Comment"));
  EXPECT_EQ(PlaylistColumn::AlbumArtist, EditTagCompleter::FieldColumn("Album artist"));
  EXPECT_EQ(PlaylistColumn::Grouping, EditTagCompleter::FieldColumn("Grouping"));

  Song a = MakeTagged("Roads", "Portishead", "Dummy");
  a.set_genre("Trip Hop");
  Song b = MakeTagged("Glory Box", "Portishead", "Dummy");
  b.set_genre("Electronic");
  const auto artists = EditTagCompleter::ValuesFor({a, b}, "Artist");
  ASSERT_EQ(1u, artists.size());
  EXPECT_EQ("Portishead", artists.front());
  const auto genres = EditTagCompleter::ValuesFor({a, b}, "Genre");
  ASSERT_EQ(2u, genres.size());
  const auto suggestions = EditTagCompleter::Suggestions({"Portishead", "Pulp", "Radiohead"}, "Po");
  ASSERT_EQ(1u, suggestions.size());
  EXPECT_EQ("Portishead", suggestions.front());
  EXPECT_TRUE(EditTagCompleter::Suggestions({"Portishead"}, "Portishead").empty());
  EXPECT_TRUE(EditTagCompleter::TagsSummary(1).empty());
  EXPECT_EQ("3 songs selected.", EditTagCompleter::TagsSummary(3));
}

TEST(EditTagSummaryFields, RowsMatchQtSummaryLabels) {
  EXPECT_STREQ("Date created", EditTagSummaryLabels::DateCreated());
  EXPECT_STREQ("Art Embedded", EditTagSummaryLabels::ArtEmbedded());
  EXPECT_STREQ("Bit rate", EditTagSummaryLabels::BitRate());
  EXPECT_STREQ("EBU R 128 integrated loudness", EditTagSummaryLabels::EbuIntegrated());
  EXPECT_STREQ("Complete tags automatically", EditTagSummaryLabels::FetchTags());
  EXPECT_STREQ("Complete lyrics automatically", EditTagSummaryLabels::FetchLyrics());

  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  song.set_filetype(Song::FileType::FLAC);
  song.set_length_nanosec(30000000000LL);
  song.set_bitrate(320);
  song.set_samplerate(44100);
  song.set_bitdepth(16);
  song.set_filesize(1024);
  song.set_art_embedded(true);
  song.set_art_unset(false);
  song.set_ebur128_integrated_loudness_lufs(-14.0);
  const auto rows = EditTagSummaryFields::Rows(song);
  ASSERT_FALSE(rows.empty());
  EXPECT_EQ("Filename", rows.front().label);
  EXPECT_EQ("Roads.flac", rows.front().value);
  bool found_embedded = false;
  bool found_rate = false;
  for (const auto &row : rows) {
    if (std::string(row.label) == EditTagSummaryLabels::ArtEmbedded()) {
      found_embedded = true;
      EXPECT_EQ("Yes", row.value);
    }
    if (std::string(row.label) == EditTagSummaryLabels::BitRate()) {
      found_rate = true;
      EXPECT_EQ("320 kbps", row.value);
    }
  }
  EXPECT_TRUE(found_embedded);
  EXPECT_TRUE(found_rate);

  Song missing;
  missing.set_filesize(-1);
  const auto unknown = EditTagSummaryFields::Rows(missing);
  bool found_unknown = false;
  for (const auto &row : unknown) {
    if (std::string(row.label) == "File size") {
      found_unknown = true;
      EXPECT_EQ("Unknown", row.value);
    }
  }
  EXPECT_TRUE(found_unknown);
}

TEST(EditTagFields, ApplyFieldWritesSortTags) {
  Song song;
  EditTagFields::ApplyField(&song, "Composer sort", "Bowie, David");
  EditTagFields::ApplyField(&song, "Performer sort", "Eno, Brian");
  EXPECT_EQ("Bowie, David", song.composersort());
  EXPECT_EQ("Eno, Brian", song.performersort());
}

TEST(EditTagFields, IsValueModifiedMatchesQtNumericUnset) {
  EXPECT_FALSE(EditTagFields::IsIntModified(-1, -1));
  EXPECT_FALSE(EditTagFields::IsIntModified(-1, 0));
  EXPECT_TRUE(EditTagFields::IsIntModified(-1, 5));
  EXPECT_TRUE(EditTagFields::IsIntModified(5, 0));
  EXPECT_TRUE(EditTagFields::IsIntModified(5, -1));
  EXPECT_FALSE(EditTagFields::IsIntModified(5, 5));
  EXPECT_TRUE(EditTagFields::IsNumericIntField("Year"));
  EXPECT_TRUE(EditTagFields::IsNumericIntField("Original year"));
  EXPECT_TRUE(EditTagFields::IsNumericIntField("Track"));
  EXPECT_TRUE(EditTagFields::IsNumericIntField("Disc"));
  EXPECT_FALSE(EditTagFields::IsNumericIntField("Title"));
  EXPECT_FALSE(EditTagFields::IsValueModified("Year", "", ""));
  EXPECT_FALSE(EditTagFields::IsValueModified("Year", "", "0"));
  EXPECT_TRUE(EditTagFields::IsValueModified("Year", "1994", "0"));
  EXPECT_TRUE(EditTagFields::IsValueModified("Year", "1994", ""));
  EXPECT_TRUE(EditTagFields::IsValueModified("Track", "8", "1"));
  EXPECT_FALSE(EditTagFields::IsValueModified("Original year", "", "0"));
  EXPECT_TRUE(EditTagFields::IsValueModified("Disc", "2", ""));
  EXPECT_FALSE(EditTagFields::IsValueModified("Title", "Roads", "Roads"));
  EXPECT_TRUE(EditTagFields::IsValueModified("Title", "Roads", "Glory Box"));
  EXPECT_FALSE(EditTagFields::IsRatingModified(-1.0, 0.0));
  EXPECT_TRUE(EditTagFields::IsRatingModified(-1.0, 0.8));
  EXPECT_TRUE(EditTagFields::IsRatingModified(0.8, 0.0));
  EXPECT_FALSE(EditTagFields::IsRatingModified(0.8, 0.8));
}

TEST(EditTagFields, NormalizeUnsetNumericClearsNonPositive) {
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  song.set_track(0);
  song.set_disc(0);
  song.set_year(0);
  song.set_originalyear(0);
  song.set_lastplayed(0);
  EditTagFields::NormalizeUnsetNumeric(&song);
  EXPECT_EQ(-1, song.track());
  EXPECT_EQ(-1, song.disc());
  EXPECT_EQ(-1, song.year());
  EXPECT_EQ(-1, song.originalyear());
  EXPECT_EQ(-1, song.lastplayed());
  song.set_track(8);
  song.set_disc(1);
  song.set_year(1994);
  song.set_originalyear(1994);
  song.set_lastplayed(1700000000);
  EditTagFields::NormalizeUnsetNumeric(&song);
  EXPECT_EQ(8, song.track());
  EXPECT_EQ(1, song.disc());
  EXPECT_EQ(1994, song.year());
  EXPECT_EQ(1994, song.originalyear());
  EXPECT_EQ(1700000000, song.lastplayed());
}

TEST(EditTagFields, ApplyFieldZeroNumericBecomesUnset) {
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  song.set_originalyear(1994);
  song.set_disc(2);
  EditTagFields::ApplyField(&song, "Year", "0");
  EditTagFields::ApplyField(&song, "Original year", "0");
  EditTagFields::ApplyField(&song, "Track", "0");
  EditTagFields::ApplyField(&song, "Disc", "0");
  EXPECT_EQ(-1, song.year());
  EXPECT_EQ(-1, song.originalyear());
  EXPECT_EQ(-1, song.track());
  EXPECT_EQ(-1, song.disc());
}

TEST(EditTagFields, CommonRatingAndSliderConversion) {
  Song unset_a = MakeTagged("Roads", "Portishead", "Dummy");
  Song unset_b = MakeTagged("Glory Box", "Portishead", "Dummy");
  unset_a.set_rating(-1.0f);
  unset_b.set_rating(-1.0f);
  EXPECT_DOUBLE_EQ(-1.0, EditTagFields::CommonRating({unset_a, unset_b}));
  EXPECT_DOUBLE_EQ(-1.0, EditTagFields::CommonRating({}));
  unset_a.set_rating(0.8f);
  unset_b.set_rating(0.8f);
  EXPECT_NEAR(0.8, EditTagFields::CommonRating({unset_a, unset_b}), 0.001);
  unset_b.set_rating(0.4f);
  EXPECT_DOUBLE_EQ(-1.0, EditTagFields::CommonRating({unset_a, unset_b}));
  EXPECT_DOUBLE_EQ(0.0, EditTagFields::RatingSliderFromStored(-1.0));
  EXPECT_NEAR(4.0, EditTagFields::RatingSliderFromStored(0.8), 0.001);
  EXPECT_NEAR(0.8f, EditTagFields::RatingStoredFromSlider(4.0), 0.001f);
}

TEST(EditTagFields, FetchTagsAndFieldEnableMatchQt) {
  EXPECT_TRUE(EditTagFields::FetchTagsEnabled(true, false));
  EXPECT_FALSE(EditTagFields::FetchTagsEnabled(true, true));
  EXPECT_FALSE(EditTagFields::FetchTagsEnabled(false, false));
  EXPECT_FALSE(EditTagFields::FetchTagsEnabled(false, true));
  EXPECT_TRUE(EditTagFields::FieldsEnabled(false, true));
  EXPECT_FALSE(EditTagFields::FieldsEnabled(true, true));
  EXPECT_FALSE(EditTagFields::FieldsEnabled(false, false));
  EXPECT_TRUE(EditTagFields::ButtonsEnabled(false));
  EXPECT_FALSE(EditTagFields::ButtonsEnabled(true));
  EXPECT_FALSE(EditTagFields::SongListVisible(0));
  EXPECT_FALSE(EditTagFields::SongListVisible(1));
  EXPECT_TRUE(EditTagFields::SongListVisible(2));
  EXPECT_TRUE(EditTagFields::SongListEnabled(false, true));
  EXPECT_FALSE(EditTagFields::SongListEnabled(true, true));
  EXPECT_FALSE(EditTagFields::SongListEnabled(false, false));
  EXPECT_TRUE(EditTagFields::SongListNavEnabled(true, false));
  EXPECT_FALSE(EditTagFields::SongListNavEnabled(true, true));
  EXPECT_FALSE(EditTagFields::SongListNavEnabled(false, false));
  EXPECT_TRUE(EditTagFields::LoadingLabelVisible(true));
  EXPECT_FALSE(EditTagFields::LoadingLabelVisible(false));
  EXPECT_STREQ("Loading tracks...", EditTagFields::LoadingTracksMessage());
  EXPECT_STREQ("Saving tracks...", EditTagFields::SavingTracksMessage());

  Song collection = MakeTagged("Roads", "Portishead", "Dummy");
  Song stream(Song::Source::Tidal);
  stream.set_valid(true);
  stream.set_url("https://tidal.example/roads");
  const SongList valid = EditTagFields::ValidSongs({collection, stream});
  ASSERT_EQ(2u, valid.size());
  EXPECT_EQ("Roads", valid.front().title());
  EXPECT_EQ(1u, EditTagFields::ValidSongs({stream}).size());
}

TEST(EditTagLoading, FileViewPlaceholdersAndLoadDataMatchQt) {
  const Song placeholder = EditTagLoading::PlaceholderFromPath("/tmp/roads.mp3");
  EXPECT_TRUE(placeholder.is_valid());
  EXPECT_EQ(Song::Source::LocalFile, placeholder.source());
  EXPECT_EQ(Song::FileType::MPEG, placeholder.filetype());
  EXPECT_TRUE(placeholder.IsEditable());
  EXPECT_TRUE(placeholder.title().empty());
  EXPECT_TRUE(EditTagLoading::OpensDialog({placeholder}));
  EXPECT_FALSE(EditTagLoading::OpensDialog({}));

  char dir_template[] = "/tmp/strawberry-edittag-XXXXXX";
  const std::string dir = mkdtemp(dir_template);
  const std::string audio = FileUtils::Join(dir, "song.mp3");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "audio"));
  const SongList placeholders = EditTagLoading::PlaceholdersFromPaths({audio, dir, ""});
  ASSERT_EQ(1u, placeholders.size());
  EXPECT_TRUE(placeholders.front().IsEditable());

  Song incoming = placeholders.front();
  incoming.set_skipcount(3);
  incoming.set_art_manual("/covers/dummy.jpg");
  incoming.set_playcount(9);
  incoming.set_rating(0.8f);

  const SongList loaded = EditTagLoading::LoadData({incoming}, [&](const std::string &path, Song *song) {
    EXPECT_EQ(audio, path);
    song->set_valid(true);
    song->set_title("Roads");
    song->set_playcount(1);
    song->set_rating(0.2f);
    return TagReaderResult{TagReaderResult::ErrorCode::Success};
  });
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("Roads", loaded.front().title());
  EXPECT_EQ(1u, loaded.front().playcount());
  EXPECT_NEAR(0.2f, loaded.front().rating(), 0.001f);
  EXPECT_EQ(3u, loaded.front().skipcount());
  EXPECT_EQ("/covers/dummy.jpg", loaded.front().art_manual());

  Song stream(Song::Source::Tidal);
  stream.set_valid(true);
  stream.set_url("https://tidal.example/roads");
  stream.set_title("Roads");
  EXPECT_FALSE(EditTagLoading::ShouldRereadFromDisk(stream));
  EXPECT_TRUE(EditTagLoading::ShouldRereadFromDisk(placeholder));
  const SongList stream_loaded = EditTagLoading::LoadData({stream}, [&](const std::string &, Song *) {
    ADD_FAILURE() << "stream metadata should not be reread from disk";
    return TagReaderResult{TagReaderResult::ErrorCode::Success};
  });
  ASSERT_EQ(1u, stream_loaded.size());
  EXPECT_EQ("Roads", stream_loaded.front().title());
  EXPECT_EQ(stream.url(), stream_loaded.front().url());

  Song invalid;
  const SongList dropped = EditTagLoading::LoadData(placeholders, [&](const std::string &, Song *song) {
    *song = invalid;
    return TagReaderResult{TagReaderResult::ErrorCode::Success};
  });
  EXPECT_TRUE(dropped.empty());
  EXPECT_FALSE(EditTagLoading::KeepRead(TagReaderResult{TagReaderResult::ErrorCode::Unsupported}, placeholder));

  FileUtils::Remove(audio);
  rmdir(dir.c_str());
}

TEST(EditTagSave, PlaylistReloadAndStreamApplyMatchQt) {
  Song local = MakeTagged("Roads", "Portishead", "Dummy");
  Song other = MakeTagged("Glory Box", "Portishead", "Dummy");
  other.set_url("file:///tmp/music/Glory Box.flac");
  Song tidal(Song::Source::Tidal);
  tidal.set_valid(true);
  tidal.set_url("https://tidal.example/roads");
  tidal.set_title("Roads");
  Song radio(Song::Source::Stream);
  radio.set_valid(true);
  radio.set_url("http://example.invalid/live");

  EXPECT_FALSE(EditTagSave::ShouldApplyStreamMetadata(local));
  EXPECT_TRUE(EditTagSave::ShouldReloadFromDisk(local));
  EXPECT_TRUE(EditTagSave::ShouldWriteFile(local));
  EXPECT_TRUE(EditTagSave::ShouldApplyStreamMetadata(tidal));
  EXPECT_FALSE(EditTagSave::ShouldReloadFromDisk(tidal));
  EXPECT_FALSE(EditTagSave::ShouldWriteFile(tidal));
  EXPECT_FALSE(EditTagSave::ShouldApplyStreamMetadata(radio));
  EXPECT_TRUE(EditTagSave::ShouldReloadFromDisk(radio));
  EXPECT_FALSE(EditTagSave::ShouldWriteFile(radio));
  EXPECT_FALSE(EditTagSave::HasPlaylistSource({}));
  EXPECT_TRUE(EditTagSave::ShouldPersist({2, 5}));

  const std::vector<int> kept = EditTagSave::RowsForValidSongs({local, tidal, other}, {4, 7, 9});
  ASSERT_EQ(3u, kept.size());
  EXPECT_EQ(4, kept[0]);
  EXPECT_EQ(7, kept[1]);
  EXPECT_EQ(9, kept[2]);
  const std::vector<int> stream_only = EditTagSave::RowsForValidSongs({tidal}, {3});
  ASSERT_EQ(1u, stream_only.size());
  EXPECT_EQ(3, stream_only.front());

  const std::vector<int> after_load = EditTagSave::RowsForLoaded({local, other}, {4, 9}, {other});
  ASSERT_EQ(1u, after_load.size());
  EXPECT_EQ(9, after_load.front());
  EXPECT_TRUE(EditTagSave::RowsForLoaded({local}, {4}, {}).empty());
  const std::vector<int> unmatched = EditTagSave::RowsForLoaded({local}, {4}, {tidal});
  ASSERT_EQ(1u, unmatched.size());
  EXPECT_EQ(-1, unmatched.front());

  const SongList playlist_songs = {tidal, local, other};
  EXPECT_EQ(1, EditTagSave::ResolveRow(playlist_songs, 1, local.url()));
  EXPECT_EQ(2, EditTagSave::ResolveRow(playlist_songs, 0, other.url()));
  EXPECT_EQ(-1, EditTagSave::ResolveRow(playlist_songs, 1, "file:///missing.flac"));
  EXPECT_EQ(-1, EditTagSave::ResolveRow(playlist_songs, 1, {}));

  const std::vector<int> resolved = EditTagSave::ResolvedRows(playlist_songs, {1, 0}, {local, other});
  ASSERT_EQ(2u, resolved.size());
  EXPECT_EQ(1, resolved[0]);
  EXPECT_EQ(2, resolved[1]);

  const std::vector<int> reload = EditTagSave::RowsToReload({1, 0, -1}, {local, tidal, other});
  ASSERT_EQ(1u, reload.size());
  EXPECT_EQ(1, reload.front());

  const auto stream_updates = EditTagSave::StreamMetadataUpdates({0, 1}, {tidal, local});
  ASSERT_EQ(1u, stream_updates.size());
  EXPECT_EQ(0, stream_updates.front().first);
  EXPECT_EQ(tidal.url(), stream_updates.front().second.url());

  SongList selected;
  std::vector<int> aligned;
  EditTagSave::CollectPlaylistSelection(playlist_songs, {1, 8, 2, -1}, &selected, &aligned);
  ASSERT_EQ(2u, selected.size());
  ASSERT_EQ(2u, aligned.size());
  EXPECT_EQ(local.url(), selected[0].url());
  EXPECT_EQ(other.url(), selected[1].url());
  EXPECT_EQ(1, aligned[0]);
  EXPECT_EQ(2, aligned[1]);
}

TEST(EditTagCover, ChangeArtAndActionEnableMatchQt) {
  Song collection = MakeTagged("Roads", "Portishead", "Dummy");
  EXPECT_TRUE(EditTagCover::ChangeArtEnabled(collection));
  EXPECT_TRUE(EditTagCover::ChangeArtEnabledWithAlbum(collection));

  Song local(Song::Source::LocalFile);
  local.set_valid(true);
  local.set_url("file:///tmp/roads.flac");
  local.set_artist("Portishead");
  local.set_album("Dummy");
  EXPECT_FALSE(EditTagCover::ChangeArtEnabled(local));
  EXPECT_FALSE(EditTagCover::ChangeArtEnabledWithAlbum(local));

  Song missing_album = MakeTagged("Roads", "Portishead", "");
  EXPECT_TRUE(EditTagCover::ChangeArtEnabled(missing_album));
  EXPECT_FALSE(EditTagCover::ChangeArtEnabledWithAlbum(missing_album));

  EXPECT_FALSE(EditTagCover::HasValidArt(collection));
  EXPECT_FALSE(EditTagCover::ShowCoverEnabled(collection, false));
  EXPECT_FALSE(EditTagCover::SaveCoverEnabled(collection, false));
  collection.set_art_embedded(true);
  EXPECT_TRUE(EditTagCover::HasValidArt(collection));
  EXPECT_TRUE(EditTagCover::ShowCoverEnabled(collection, false));
  EXPECT_TRUE(EditTagCover::ShowOnDoubleClick(collection, false));
  EXPECT_TRUE(EditTagCover::SaveCoverEnabled(collection, false));
  EXPECT_FALSE(EditTagCover::ShowCoverEnabled(collection, true));
  EXPECT_FALSE(EditTagCover::ShowOnDoubleClick(collection, true));
  collection.set_art_unset(true);
  EXPECT_FALSE(EditTagCover::ShowCoverEnabled(collection, false));

  const bool change_art = true;
  EXPECT_TRUE(EditTagCover::FetchCoverEnabled(change_art));
  EXPECT_FALSE(EditTagCover::FetchCoverEnabled(false));
  EXPECT_TRUE(EditTagCover::FromFileEnabled(change_art));
  EXPECT_TRUE(EditTagCover::FromUrlEnabled(change_art));
  EXPECT_FALSE(EditTagCover::FromFileEnabled(false));
  EXPECT_TRUE(EditTagCover::SearchCoverEnabled(true, change_art, false));
  EXPECT_FALSE(EditTagCover::SearchCoverEnabled(false, change_art, false));
  EXPECT_TRUE(EditTagCover::SearchCoverEnabled(false, change_art, true));
  EXPECT_FALSE(EditTagCover::SearchCoverEnabled(true, false, false));

  Song art;
  art.set_source(Song::Source::Collection);
  EXPECT_TRUE(EditTagCover::UnsetCoverEnabled(art, change_art, false));
  art.set_art_unset(true);
  EXPECT_FALSE(EditTagCover::UnsetCoverEnabled(art, change_art, false));
  EXPECT_TRUE(EditTagCover::UnsetCoverEnabled(art, change_art, true));
  EXPECT_TRUE(EditTagCover::ClearCoverEnabled(art, change_art, false));
  art.set_art_unset(false);
  EXPECT_FALSE(EditTagCover::ClearCoverEnabled(art, change_art, false));
  art.set_art_manual("/covers/dummy.jpg");
  EXPECT_TRUE(EditTagCover::ClearCoverEnabled(art, change_art, false));
  EXPECT_TRUE(EditTagCover::DeleteCoverEnabled(art, change_art, false));
  art.set_art_manual("");
  EXPECT_FALSE(EditTagCover::DeleteCoverEnabled(art, change_art, false));
  art.set_art_embedded(true);
  EXPECT_TRUE(EditTagCover::DeleteCoverEnabled(art, change_art, false));
  EXPECT_FALSE(EditTagCover::DeleteCoverEnabled(art, false, false));
}

TEST(EditTagCover, ArtDifferentAndTabEnableMatchQt) {
  EXPECT_STREQ("Different art across multiple songs.", EditTagCover::ArtDifferentHint());
  EXPECT_STREQ("(different across multiple songs)", EditTagCover::TagsDifferentHint());
  EXPECT_TRUE(EditTagCover::SummaryTabEnabled(1));
  EXPECT_TRUE(EditTagCover::LyricsTabEnabled(0));
  EXPECT_TRUE(EditTagCover::LyricsTabEnabled(1));
  EXPECT_FALSE(EditTagCover::SummaryTabEnabled(2));
  EXPECT_FALSE(EditTagCover::LyricsTabEnabled(2));

  Song a = MakeTagged("Roads", "Portishead", "Dummy");
  Song b = MakeTagged("Glory Box", "Portishead", "Dummy");
  EXPECT_FALSE(EditTagCover::ArtDifferent(a, b));
  EXPECT_FALSE(EditTagCover::ArtDifferentAcrossSongs({a}));
  EXPECT_FALSE(EditTagCover::ArtDifferentAcrossSongs({a, b}));
  b.set_art_manual("/covers/glory.jpg");
  EXPECT_TRUE(EditTagCover::ArtDifferent(a, b));
  EXPECT_TRUE(EditTagCover::ArtDifferentAcrossSongs({a, b}));
  a.set_art_embedded(true);
  b.set_art_manual("");
  b.set_art_embedded(true);
  EXPECT_FALSE(EditTagCover::ArtDifferent(a, b));
  b.set_album("Portishead");
  EXPECT_TRUE(EditTagCover::ArtDifferent(a, b));
  EXPECT_TRUE(EditTagCover::ShowCoverEnabled(a, false));
  EXPECT_FALSE(EditTagCover::ShowCoverEnabled(a, EditTagCover::ArtDifferentAcrossSongs({a, b})));
}

TEST(EditTagCover, TagsArtGesturesMatchQt) {
  EXPECT_STREQ("Change art", EditTagCover::ChangeArt());
  EXPECT_EQ(160, EditTagCover::kSummaryArtSize);
  EXPECT_EQ(128, EditTagCover::kTagsArtSize);
  EXPECT_EQ(1, EditTagCover::kPrimaryButton);
  EXPECT_TRUE(EditTagCover::IsDoubleClick(2, 1));
  EXPECT_TRUE(EditTagCover::IsDoubleClick(3, 1));
  EXPECT_FALSE(EditTagCover::IsDoubleClick(1, 1));
  EXPECT_FALSE(EditTagCover::IsDoubleClick(2, 3));
  EXPECT_TRUE(EditTagCover::IsMenuClick(1, 1));
  EXPECT_FALSE(EditTagCover::IsMenuClick(2, 1));
  EXPECT_FALSE(EditTagCover::IsMenuClick(1, 3));
  EXPECT_STREQ("Complete tags automatically", EditTagSummaryLabels::FetchTags());
}

TEST(EditTagCover, TagsTabLayoutMatchesQt) {
  EXPECT_TRUE(EditTagCover::RatingOnTagsTab());
  EXPECT_TRUE(EditTagCover::CoreFieldsOnTagsTab());
  EXPECT_TRUE(EditTagCover::EmbeddedCoverOnTagsTab());
  EXPECT_STREQ("/org/strawberrymusicplayer/Strawberry/pictures/musicbrainz.png", EditTagSummaryLabels::MusicBrainzIconResource());
  EXPECT_EQ(38, EditTagSummaryLabels::kMusicBrainzIconWidth);
  EXPECT_EQ(22, EditTagSummaryLabels::kMusicBrainzIconHeight);
}

TEST(EditTagTabs, VisibleIndexSkipsDisabledTabs) {
  EXPECT_EQ(0, EditTagTabs::VisibleIndex(0, 1));
  EXPECT_EQ(1, EditTagTabs::VisibleIndex(0, 2));
  EXPECT_EQ(1, EditTagTabs::VisibleIndex(1, 2));
  EXPECT_EQ(1, EditTagTabs::VisibleIndex(2, 2));
  EXPECT_EQ(2, EditTagTabs::VisibleIndex(2, 1));
  EXPECT_EQ(3, EditTagTabs::VisibleIndex(3, 2));
}

TEST(EditTagFields, FieldEnabledMatchesQtFiletypeSupport) {
  Song flac;
  flac.set_filetype(Song::FileType::FLAC);
  Song mpeg;
  mpeg.set_filetype(Song::FileType::MPEG);
  Song mp4;
  mp4.set_filetype(Song::FileType::MP4);
  Song asf;
  asf.set_filetype(Song::FileType::ASF);
  Song dsf;
  dsf.set_filetype(Song::FileType::DSF);
  Song opus;
  opus.set_filetype(Song::FileType::OggOpus);

  EXPECT_TRUE(flac.additional_tags_supported());
  EXPECT_FALSE(dsf.additional_tags_supported());
  EXPECT_FALSE(asf.additional_tags_supported());
  EXPECT_TRUE(asf.albumartist_supported());
  EXPECT_FALSE(dsf.albumartist_supported());
  EXPECT_FALSE(asf.performer_supported());
  EXPECT_TRUE(mpeg.performer_supported());
  EXPECT_FALSE(mp4.performer_supported());
  EXPECT_TRUE(asf.lyrics_supported());
  EXPECT_FALSE(asf.grouping_supported());
  EXPECT_FALSE(asf.compilation_supported());
  EXPECT_FALSE(asf.comment_supported());
  EXPECT_TRUE(asf.rating_supported());
  EXPECT_FALSE(dsf.rating_supported());
  EXPECT_TRUE(flac.performersort_supported());
  EXPECT_FALSE(mpeg.performersort_supported());
  EXPECT_FALSE(opus.performersort_supported());
  EXPECT_TRUE(opus.titlesort_supported());
  EXPECT_FALSE(mp4.titlesort_supported());

  EXPECT_TRUE(EditTagFields::FieldEnabled("Title", {dsf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Artist", {dsf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Year", {dsf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Album artist", {dsf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Album artist", {asf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Album artist", {flac, dsf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Composer", {dsf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Composer", {asf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Performer", {asf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Performer", {mpeg}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Grouping", {asf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Genre", {asf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Compilation", {asf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Comment", {dsf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Lyrics", {dsf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Lyrics", {asf}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Rating", {dsf}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Rating", {mp4}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Performer sort", {mpeg}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Performer sort", {flac}));
  EXPECT_TRUE(EditTagFields::FieldEnabled("Title sort", {mpeg}));
  EXPECT_FALSE(EditTagFields::FieldEnabled("Title sort", {mp4}));
  EXPECT_TRUE(EditTagFields::AnySupported({flac, dsf}, &Song::albumartist_supported));
  EXPECT_FALSE(EditTagFields::AnySupported({dsf}, &Song::albumartist_supported));
}

TEST(EditTagSave, WriteFailureMessageMatchesQt) {
  EXPECT_EQ("Could not write metadata to /tmp/track.flac", EditTagSave::WriteFailureMessage("/tmp/track.flac"));
  EXPECT_EQ("Could not write metadata to /tmp/track.flac: File is read-only",
            EditTagSave::WriteFailureMessage("/tmp/track.flac", "File is read-only"));
}

TEST(TagWriterFields, WritesBpmMoodKeyAndOriginalYear) {
  EXPECT_STREQ("BPM", TagWriterFields::VorbisBpm());
  EXPECT_STREQ("MOOD", TagWriterFields::VorbisMood());
  EXPECT_STREQ("INITIALKEY", TagWriterFields::VorbisInitialKey());
  EXPECT_STREQ("ORIGINALYEAR", TagWriterFields::VorbisOriginalYear());
  EXPECT_STREQ("TBPM", TagWriterFields::Id3Bpm());
  EXPECT_STREQ("TKEY", TagWriterFields::Id3InitialKey());
  EXPECT_STREQ("TORY", TagWriterFields::Id3OriginalYear());
  EXPECT_STREQ("MOOD", TagWriterFields::Id3Mood());
  EXPECT_STREQ("tmpo", TagWriterFields::Mp4Bpm());
  EXPECT_STREQ("----:com.apple.iTunes:MOOD", TagWriterFields::Mp4Mood());
  EXPECT_STREQ("----:com.apple.iTunes:initialkey", TagWriterFields::Mp4InitialKey());
  EXPECT_STREQ("----:com.apple.iTunes:originalyear", TagWriterFields::Mp4OriginalYear());
  EXPECT_TRUE(TagWriterFields::HasBpm(128.0f));
  EXPECT_FALSE(TagWriterFields::HasBpm(0.0f));
  EXPECT_TRUE(TagWriterFields::HasOriginalYear(1994));
  EXPECT_FALSE(TagWriterFields::HasOriginalYear(0));
}
