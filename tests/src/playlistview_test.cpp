#include "playlist/playlistdelegates.h"
#include "playlist/playlist.h"
#include "playlist/playlistlistmodel.h"
#include "playlist/playlistlistsortfiltermodel.h"
#include "playlist/playlistsaveoptionsdialog.h"
#include "playlist/songloaderinserter.h"
#include "widgets/ratingpainter.h"

#include <gtest/gtest.h>

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
