#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlist.h"
#include "playlist/playlistlistdrop.h"
#include "playlist/playlistlistmodel.h"
#include "playlist/playlistlistsortfiltermodel.h"
#include "playlist/playlistsaveoptionsdialog.h"
#include "playlist/songloaderinserter.h"
#include "playlist/playlistratingclick.h"
#include "widgets/ratingpainter.h"

#include <gtest/gtest.h>

TEST(PlaylistColumnLayout, DefaultOrderAlignmentAndReset) {
  PlaylistColumnLayout::Reset();
  const std::vector<PlaylistColumn> defaults = PlaylistColumnLayout::DefaultVisible();
  ASSERT_GE(defaults.size(), 5u);
  EXPECT_EQ(PlaylistColumn::Track, defaults.front());
  EXPECT_EQ(defaults, PlaylistColumnLayout::Visible());
  EXPECT_TRUE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Comment));
  EXPECT_EQ(PlaylistColumnAlign::Right, PlaylistColumnLayout::DefaultAlignment(PlaylistColumn::Track));
  EXPECT_EQ(PlaylistColumnAlign::Right, PlaylistColumnLayout::Alignment(PlaylistColumn::Year));
  EXPECT_EQ(PlaylistColumnAlign::Left, PlaylistColumnLayout::Alignment(PlaylistColumn::Title));
  EXPECT_FLOAT_EQ(1.0f, PlaylistColumnLayout::XAlign(PlaylistColumn::Bitrate));
  EXPECT_FLOAT_EQ(0.0f, PlaylistColumnLayout::XAlign(PlaylistColumn::Artist));
  EXPECT_TRUE(PlaylistColumnLayout::StretchEnabled());
  EXPECT_TRUE(PlaylistColumnLayout::StretchColumn(PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistColumnLayout::StretchColumn(PlaylistColumn::Track));
  EXPECT_FALSE(PlaylistColumnLayout::RatingLocked());

  PlaylistColumnLayout::SetAlignment(PlaylistColumn::Title, PlaylistColumnAlign::Center);
  EXPECT_EQ(PlaylistColumnAlign::Center, PlaylistColumnLayout::Alignment(PlaylistColumn::Title));
  EXPECT_FLOAT_EQ(0.5f, PlaylistColumnLayout::XAlign(PlaylistColumn::Title));
  PlaylistColumnLayout::SetStretchEnabled(false);
  EXPECT_FALSE(PlaylistColumnLayout::StretchEnabled());
  EXPECT_FALSE(PlaylistColumnLayout::StretchColumn(PlaylistColumn::Title));
  PlaylistColumnLayout::SetRatingLocked(true);
  EXPECT_TRUE(PlaylistColumnLayout::RatingLocked());

  PlaylistColumnLayout::Hide(PlaylistColumn::Track);
  EXPECT_FALSE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Track));
  EXPECT_EQ(PlaylistColumn::Title, PlaylistColumnLayout::Visible().front());
  PlaylistColumnLayout::ToggleVisible(PlaylistColumn::Comment);
  EXPECT_TRUE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Comment));
  ASSERT_TRUE(PlaylistColumnLayout::Move(PlaylistColumn::Title, 1));
  EXPECT_NE(PlaylistColumn::Title, PlaylistColumnLayout::Visible().front());

  PlaylistColumnLayout::SetVisibleColumns({PlaylistColumn::Title});
  PlaylistColumnLayout::Hide(PlaylistColumn::Title);
  EXPECT_TRUE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Title));

  PlaylistColumnLayout::Reset();
  EXPECT_EQ(defaults, PlaylistColumnLayout::Visible());
  EXPECT_EQ(PlaylistColumnAlign::Left, PlaylistColumnLayout::Alignment(PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistColumnLayout::StretchEnabled());
  EXPECT_FALSE(PlaylistColumnLayout::RatingLocked());
}

TEST(PlaylistDelegates, ColumnTitleAndText) {
  EXPECT_EQ("Title", PlaylistDelegates::ColumnTitle(PlaylistColumn::Title));
  EXPECT_EQ("Album artist", PlaylistDelegates::ColumnTitle(PlaylistColumn::AlbumArtist));
  EXPECT_EQ("Sample rate", PlaylistDelegates::ColumnTitle(PlaylistColumn::Samplerate));
  EXPECT_EQ(200, PlaylistDelegates::ColumnWidth(PlaylistColumn::Title));
  EXPECT_EQ(80, PlaylistDelegates::ColumnWidth(PlaylistColumn::Track));

  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_albumartist("Portishead");
  song.set_year(1994);
  song.set_track(8);
  song.set_length_nanosec(30000000000LL);
  song.set_bitrate(320);
  song.set_rating(1.0f);
  song.set_url("file:///tmp/roads.flac");
  song.set_valid(true);
  EXPECT_EQ("Roads", PlaylistDelegates::ColumnText(song, PlaylistColumn::Title));
  EXPECT_EQ("Portishead", PlaylistDelegates::ColumnText(song, PlaylistColumn::Artist));
  EXPECT_EQ("8", PlaylistDelegates::ColumnText(song, PlaylistColumn::Track));
  EXPECT_EQ("1994", PlaylistDelegates::ColumnText(song, PlaylistColumn::Year));
  EXPECT_EQ("320", PlaylistDelegates::ColumnText(song, PlaylistColumn::Bitrate));
  EXPECT_EQ("★★★★★", PlaylistDelegates::ColumnText(song, PlaylistColumn::Rating));
  EXPECT_EQ("roads.flac", PlaylistDelegates::ColumnText(song, PlaylistColumn::Filename));
}

TEST(PlaylistDelegates, EditableColumnsMatchQt) {
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::TitleSort));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Artist));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Album));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::AlbumArtist));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Composer));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Performer));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Grouping));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Track));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Disc));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Year));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Genre));
  EXPECT_TRUE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Comment));
  EXPECT_FALSE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Length));
  EXPECT_FALSE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Bitrate));
  EXPECT_FALSE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::URL));
  EXPECT_FALSE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Filename));
  EXPECT_FALSE(PlaylistDelegates::ColumnIsEditable(PlaylistColumn::Rating));

  Song song;
  song.set_title("Old");
  song.set_artist("A");
  song.set_year(1990);
  song.set_track(1);
  EXPECT_TRUE(PlaylistDelegates::SetColumnValue(song, PlaylistColumn::Title, "New"));
  EXPECT_EQ("New", song.title());
  EXPECT_TRUE(PlaylistDelegates::SetColumnValue(song, PlaylistColumn::Artist, "B"));
  EXPECT_EQ("B", song.artist());
  EXPECT_TRUE(PlaylistDelegates::SetColumnValue(song, PlaylistColumn::Year, "2001"));
  EXPECT_EQ(2001, song.year());
  EXPECT_TRUE(PlaylistDelegates::SetColumnValue(song, PlaylistColumn::Track, ""));
  EXPECT_EQ(-1, song.track());
  EXPECT_TRUE(PlaylistDelegates::SetColumnValue(song, PlaylistColumn::TitleSort, "New sort"));
  EXPECT_EQ("New sort", song.titlesort());
  EXPECT_FALSE(PlaylistDelegates::SetColumnValue(song, PlaylistColumn::Bitrate, "320"));
  EXPECT_EQ(-1, song.bitrate());
}

TEST(PlaylistDelegates, RatingStars) {
  EXPECT_TRUE(PlaylistDelegates::RatingStars(-1.0f).empty());
  EXPECT_EQ("★★★★★", PlaylistDelegates::RatingStars(1.0f));
  EXPECT_EQ("★★★☆☆", PlaylistDelegates::RatingStars(0.6f));
}

TEST(RatingPainter, MapsPositionAndStars) {
  EXPECT_EQ(5, RatingPainter::kStarCount);
  EXPECT_FLOAT_EQ(-1.0f, RatingPainter::RatingForPos(10, 0));
  EXPECT_FLOAT_EQ(0.0f, RatingPainter::RatingForPos(0, 100));
  EXPECT_FLOAT_EQ(1.0f, RatingPainter::RatingForPos(100, 100));
  EXPECT_EQ(0, RatingPainter::StarCount(-1.0f));
  EXPECT_EQ(5, RatingPainter::StarCount(1.0f));
  EXPECT_EQ(3, RatingPainter::StarCount(0.6f));
  EXPECT_TRUE(RatingPainter::Stars(-1.0f).empty());
  EXPECT_EQ("★★★★★", RatingPainter::Stars(1.0f));
}

TEST(PlaylistRatingClick, HonorsLockAndMapsStars) {
  EXPECT_TRUE(PlaylistRatingClick::IsRatingColumn(PlaylistColumn::Rating));
  EXPECT_FALSE(PlaylistRatingClick::IsRatingColumn(PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistRatingClick::CanApply(false));
  EXPECT_FALSE(PlaylistRatingClick::CanApply(true));
  EXPECT_FLOAT_EQ(1.0f, PlaylistRatingClick::RatingFromClick(100, 100));
  float rating = -1.0f;
  EXPECT_TRUE(PlaylistRatingClick::ShouldRate(PlaylistColumn::Rating, false, 100, 100, &rating));
  EXPECT_FLOAT_EQ(1.0f, rating);
  EXPECT_FALSE(PlaylistRatingClick::ShouldRate(PlaylistColumn::Rating, true, 100, 100, &rating));
  EXPECT_FALSE(PlaylistRatingClick::ShouldRate(PlaylistColumn::Title, false, 100, 100, &rating));
  EXPECT_FALSE(PlaylistRatingClick::ShouldRate(PlaylistColumn::Rating, false, 10, 0, &rating));
}

TEST(PlaylistListModel, ReloadAndLookup) {
  PlaylistListModel model;
  model.Reload(nullptr);
  EXPECT_EQ(0, model.Count());
  model.SetRows({"Queue", "Favorites", "Inbox"}, {false, true, false});
  EXPECT_EQ(3, model.Count());
  EXPECT_EQ("Favorites", model.At(1));
  EXPECT_EQ(1, model.IndexOf("Favorites"));
  EXPECT_EQ(-1, model.IndexOf("Missing"));
  EXPECT_TRUE(model.favorites()[1]);
}

TEST(PlaylistListSortFilterModel, FiltersAndSorts) {
  PlaylistListModel model;
  model.SetRows({"Zebra", "Apple", "Favorites"}, {false, false, true});
  PlaylistListSortFilterModel filter(&model);
  std::vector<std::string> all = filter.Visible();
  ASSERT_EQ(3u, all.size());
  EXPECT_EQ("Apple", all[0]);
  EXPECT_EQ("Favorites", all[1]);
  EXPECT_EQ("Zebra", all[2]);

  filter.SetFilter("fav");
  std::vector<std::string> fav = filter.Visible();
  ASSERT_EQ(1u, fav.size());
  EXPECT_EQ("Favorites", fav[0]);

  filter.SetFilter({});
  filter.SetFavoritesOnly(true);
  std::vector<std::string> starred = filter.Visible();
  ASSERT_EQ(1u, starred.size());
  EXPECT_EQ("Favorites", starred[0]);

  filter.SetFavoritesOnly(false);
  const std::vector<PlaylistListDrop::Row> rows = filter.VisibleRows();
  ASSERT_EQ(3u, rows.size());
  EXPECT_EQ("Apple", rows[0].name);
  EXPECT_FALSE(rows[0].favorite);
  EXPECT_EQ("Favorites", rows[1].name);
  EXPECT_TRUE(rows[1].favorite);
}

TEST(PlaylistListDrop, DisplayNameAndPayloads) {
  EXPECT_EQ("Inbox", PlaylistListDrop::DisplayName("Inbox", false));
  EXPECT_EQ("★ Favorites", PlaylistListDrop::DisplayName("Favorites", true));
  EXPECT_TRUE(PlaylistListDrop::IsPlaylistRows("strawberry-playlist-rows:0,2"));
  const std::vector<int> rows = PlaylistListDrop::ParsePlaylistRows("strawberry-playlist-rows:0,2");
  ASSERT_EQ(2u, rows.size());
  EXPECT_EQ(0, rows[0]);
  EXPECT_EQ(2, rows[1]);
  const std::vector<std::string> urls = PlaylistListDrop::ParseUrls("file:///a\nfile:///b");
  ASSERT_EQ(2u, urls.size());
  EXPECT_EQ("file:///a", urls[0]);
}

TEST(PlaylistSaveOptionsDialog, PathTypeLabels) {
  EXPECT_STREQ("Automatic", PlaylistSaveOptionsDialog::Label(PlaylistSaveOptionsDialog::PathType::Automatic));
  EXPECT_STREQ("Relative paths", PlaylistSaveOptionsDialog::Label(PlaylistSaveOptionsDialog::PathType::Relative));
  EXPECT_STREQ("Absolute paths", PlaylistSaveOptionsDialog::Label(PlaylistSaveOptionsDialog::PathType::Absolute));
  EXPECT_STREQ("Ask every time", PlaylistSaveOptionsDialog::Label(PlaylistSaveOptionsDialog::PathType::Ask_User));
}

TEST(SongLoaderInserter, NullTagReaderReturnsEmpty) {
  SongLoaderInserter loader(nullptr);
  EXPECT_TRUE(loader.Load({"file:///tmp/missing.flac"}).empty());
  Playlist playlist;
  EXPECT_EQ(0, loader.Insert(&playlist, {"file:///tmp/missing.flac"}));
  EXPECT_EQ(0, playlist.row_count());
}
