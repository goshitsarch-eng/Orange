#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistheadersort.h"
#include "playlist/playlistautoscroll.h"
#include "playlist/playlistclipboard.h"
#include "playlist/playlistdropindicator.h"
#include "playlist/playlistheaderreorder.h"
#include "playlist/playlistkeyboard.h"
#include "playlist/playlistcolumnwidths.h"
#include "playlist/playlistplayingicon.h"
#include "playlist/playlist.h"
#include "playlist/playlisteditorder.h"
#include "playlist/playlistfolders.h"
#include "playlist/playlistlistdrop.h"
#include "playlist/playlistlistkeyboard.h"
#include "playlist/playlistlistlook.h"
#include "playlist/playlistlistmodel.h"
#include "playlist/playlistlistsortfiltermodel.h"
#include "playlist/playlistsaveoptionsdialog.h"
#include "playlist/playlisttabnav.h"
#include "playlist/playlisttabmenu.h"
#include "playlist/playlisttagcompletion.h"
#include "widgets/favoritewidget.h"
#include "playlist/songloaderinserter.h"
#include "playlist/playlistratingclick.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/ratingpainter.h"

#include <gtest/gtest.h>

TEST(PlaylistColumnLayout, DefaultOrderAlignmentAndReset) {
  PlaylistColumnLayout::Reset();
  const std::vector<PlaylistColumn> defaults = PlaylistColumnLayout::DefaultVisible();
  ASSERT_GE(defaults.size(), 5u);
  EXPECT_EQ(PlaylistColumn::Queue, defaults.front());
  EXPECT_EQ(defaults, PlaylistColumnLayout::Visible());
  EXPECT_TRUE(PlaylistColumnLayout::CanHide());
  EXPECT_TRUE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Queue));
  EXPECT_FALSE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Comment));
  EXPECT_EQ(PlaylistColumnAlign::Right, PlaylistColumnLayout::DefaultAlignment(PlaylistColumn::Track));
  EXPECT_EQ(PlaylistColumnAlign::Right, PlaylistColumnLayout::DefaultAlignment(PlaylistColumn::Queue));
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
  EXPECT_EQ(PlaylistColumn::Queue, PlaylistColumnLayout::Visible().front());
  PlaylistColumnLayout::ToggleVisible(PlaylistColumn::Comment);
  EXPECT_TRUE(PlaylistColumnLayout::IsVisible(PlaylistColumn::Comment));
  ASSERT_TRUE(PlaylistColumnLayout::Move(PlaylistColumn::Title, 1));
  EXPECT_NE(PlaylistColumn::Title, PlaylistColumnLayout::Visible().front());

  PlaylistColumnLayout::SetVisibleColumns({PlaylistColumn::Title});
  EXPECT_FALSE(PlaylistColumnLayout::CanHide());
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
  song.set_mood("melancholy");
  EXPECT_TRUE(PlaylistDelegates::ColumnText(song, PlaylistColumn::Moodbar).empty());
  EXPECT_EQ("melancholy", PlaylistDelegates::ColumnText(song, PlaylistColumn::Mood));
  EXPECT_EQ(120, PlaylistDelegates::ColumnWidth(PlaylistColumn::Moodbar));
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
  EXPECT_TRUE(PlaylistListDrop::IsPlaylistMove("strawberry-playlist-move:Inbox"));
  EXPECT_EQ("Inbox", PlaylistListDrop::ParseMoveName(PlaylistListDrop::MovePayload("Inbox")));
}

TEST(PlaylistFolders, PathHelpersAndRename) {
  EXPECT_EQ("Rock Live", PlaylistFolders::SanitizeName("Rock/Live"));
  EXPECT_EQ(std::vector<std::string>({"Jazz", "Live"}), PlaylistFolders::Split("Jazz/Live"));
  EXPECT_EQ("Jazz/Live", PlaylistFolders::Join({"Jazz", "Live"}));
  EXPECT_EQ("Jazz", PlaylistFolders::Parent("Jazz/Live"));
  EXPECT_EQ("Live", PlaylistFolders::Leaf("Jazz/Live"));
  EXPECT_EQ("Jazz/Live", PlaylistFolders::Child("Jazz", "Live"));
  EXPECT_TRUE(PlaylistFolders::IsUnder("Jazz/Live", "Jazz"));
  EXPECT_FALSE(PlaylistFolders::IsUnder("Rock", "Jazz"));
  EXPECT_EQ("Metal/Deep", PlaylistFolders::RenamePrefix("Rock/Deep", "Rock", "Metal"));
  EXPECT_EQ("Metal", PlaylistFolders::RenamePrefix("Rock", "Rock", "Metal"));
}

TEST(PlaylistFolders, FlattenBuildsTreeAndRespectsCollapse) {
  const std::vector<PlaylistFolders::PlaylistRef> playlists{
      {"Inbox", false, ""},
      {"Rock Hits", true, "Rock"},
      {"Apollo", false, "Jazz/Live"},
  };
  const auto expanded = PlaylistFolders::Flatten(playlists, {"Empty"}, {}, {}, false);
  ASSERT_EQ(7u, expanded.size());
  EXPECT_TRUE(expanded[0].folder);
  EXPECT_EQ("Empty", expanded[0].name);
  EXPECT_EQ("Inbox", expanded[1].name);
  EXPECT_FALSE(expanded[1].folder);
  EXPECT_EQ("Jazz", expanded[2].name);
  EXPECT_TRUE(expanded[2].folder);
  EXPECT_EQ(0, expanded[2].depth);
  EXPECT_EQ("Live", expanded[3].name);
  EXPECT_EQ(1, expanded[3].depth);
  EXPECT_EQ("Apollo", expanded[4].name);
  EXPECT_EQ(2, expanded[4].depth);
  EXPECT_EQ("Rock", expanded[5].name);
  EXPECT_EQ("Rock Hits", expanded[6].name);
  EXPECT_TRUE(expanded[6].favorite);

  const auto collapsed = PlaylistFolders::Flatten(playlists, {}, {"Jazz"}, {}, false);
  ASSERT_EQ(4u, collapsed.size());
  EXPECT_EQ("Inbox", collapsed[0].name);
  EXPECT_EQ("Jazz", collapsed[1].name);
  EXPECT_FALSE(collapsed[1].expanded);
  EXPECT_EQ("Rock", collapsed[2].name);
  EXPECT_EQ("Rock Hits", collapsed[3].name);

  PlaylistListModel model;
  model.SetRows({"Inbox", "Rock Hits"}, {false, true}, {"", "Rock"});
  PlaylistListSortFilterModel filter(&model);
  const auto rows = filter.VisibleRows();
  ASSERT_EQ(3u, rows.size());
  EXPECT_EQ("Inbox", rows[0].name);
  EXPECT_EQ("Rock", rows[1].name);
  EXPECT_TRUE(rows[1].folder);
  EXPECT_EQ("Rock Hits", rows[2].name);
}

TEST(PlaylistListKeyboard, FromKeyExpandCollapseAndLabels) {
  EXPECT_EQ(PlaylistListKeyboard::Action::Activate, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(PlaylistListKeyboard::Action::MoveUp, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kUp));
  EXPECT_EQ(PlaylistListKeyboard::Action::MoveDown, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(PlaylistListKeyboard::Action::Home, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kHome));
  EXPECT_EQ(PlaylistListKeyboard::Action::End, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kEnd));
  EXPECT_EQ(PlaylistListKeyboard::Action::Expand, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kRight));
  EXPECT_EQ(PlaylistListKeyboard::Action::Collapse, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kLeft));
  EXPECT_EQ(PlaylistListKeyboard::Action::Delete, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kDelete));
  EXPECT_EQ(PlaylistListKeyboard::Action::Escape, PlaylistListKeyboard::FromKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(PlaylistListKeyboard::Action::None, PlaylistListKeyboard::FromKey('a'));
  EXPECT_EQ(ListBoxKeyboard::Action::MoveDown, PlaylistListKeyboard::MoveAction(PlaylistListKeyboard::Action::MoveDown));
  EXPECT_EQ(ListBoxKeyboard::Action::None, PlaylistListKeyboard::MoveAction(PlaylistListKeyboard::Action::Expand));
  const std::vector<std::string> labels = PlaylistListKeyboard::RowLabels({
      {"Rock", false, true, "Rock", 0, true},
      {"Favorites", true, false, "Favorites", 1, true},
  });
  ASSERT_EQ(2u, labels.size());
  EXPECT_EQ("Rock", labels[0]);
  EXPECT_EQ("★ Favorites", labels[1]);
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

TEST(PlaylistEditOrder, FromKeyAndWrap) {
  EXPECT_EQ(PlaylistEditOrder::TabAction::Next, PlaylistEditOrder::FromKey(PlaylistEditOrder::kTab, false));
  EXPECT_EQ(PlaylistEditOrder::TabAction::Previous, PlaylistEditOrder::FromKey(PlaylistEditOrder::kTab, true));
  EXPECT_EQ(PlaylistEditOrder::TabAction::Previous, PlaylistEditOrder::FromKey(PlaylistEditOrder::kISOLeftTab, false));
  EXPECT_EQ(PlaylistEditOrder::TabAction::None, PlaylistEditOrder::FromKey('a', false));

  const std::vector<int> rows = {0, 2, 5};
  const std::vector<PlaylistColumn> visible = {PlaylistColumn::Queue, PlaylistColumn::Title, PlaylistColumn::Artist, PlaylistColumn::Length};
  const std::vector<PlaylistColumn> editable = PlaylistEditOrder::EditableVisible(visible);
  ASSERT_EQ(2u, editable.size());
  EXPECT_EQ(PlaylistColumn::Title, editable[0]);
  EXPECT_EQ(PlaylistColumn::Artist, editable[1]);

  const PlaylistEditOrder::Cell next = PlaylistEditOrder::Next(0, PlaylistColumn::Title, rows, editable);
  EXPECT_TRUE(next.valid);
  EXPECT_EQ(0, next.row);
  EXPECT_EQ(PlaylistColumn::Artist, next.column);

  const PlaylistEditOrder::Cell wrap_row = PlaylistEditOrder::Next(0, PlaylistColumn::Artist, rows, editable);
  EXPECT_TRUE(wrap_row.valid);
  EXPECT_EQ(2, wrap_row.row);
  EXPECT_EQ(PlaylistColumn::Title, wrap_row.column);

  const PlaylistEditOrder::Cell wrap_end = PlaylistEditOrder::Next(5, PlaylistColumn::Artist, rows, editable);
  EXPECT_TRUE(wrap_end.valid);
  EXPECT_EQ(0, wrap_end.row);
  EXPECT_EQ(PlaylistColumn::Title, wrap_end.column);

  const PlaylistEditOrder::Cell prev = PlaylistEditOrder::Previous(2, PlaylistColumn::Title, rows, editable);
  EXPECT_TRUE(prev.valid);
  EXPECT_EQ(0, prev.row);
  EXPECT_EQ(PlaylistColumn::Artist, prev.column);

  const PlaylistEditOrder::Cell wrap_start = PlaylistEditOrder::Previous(0, PlaylistColumn::Title, rows, editable);
  EXPECT_TRUE(wrap_start.valid);
  EXPECT_EQ(5, wrap_start.row);
  EXPECT_EQ(PlaylistColumn::Artist, wrap_start.column);

  EXPECT_FALSE(PlaylistEditOrder::Next(0, PlaylistColumn::Title, {}, editable).valid);
  EXPECT_FALSE(PlaylistEditOrder::Previous(0, PlaylistColumn::Title, rows, {}).valid);
}

TEST(PlaylistTagCompletion, ColumnsAndUniqueValues) {
  EXPECT_TRUE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::Album));
  EXPECT_TRUE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::Artist));
  EXPECT_TRUE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::Genre));
  EXPECT_TRUE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::TitleSort));
  EXPECT_FALSE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::Year));
  EXPECT_FALSE(PlaylistTagCompletion::CompletesColumn(PlaylistColumn::Comment));

  Song a;
  a.set_artist("Portishead");
  a.set_album("Dummy");
  a.set_genre("Trip Hop");
  Song b;
  b.set_artist("portishead");
  b.set_album("Third");
  b.set_genre("Trip Hop");
  Song c;
  c.set_artist("Massive Attack");
  c.set_album("");
  const std::vector<std::string> albums = PlaylistTagCompletion::UniqueValues({a, b, c}, PlaylistColumn::Album);
  ASSERT_EQ(2u, albums.size());
  EXPECT_EQ("Dummy", albums[0]);
  EXPECT_EQ("Third", albums[1]);
  EXPECT_TRUE(PlaylistTagCompletion::UniqueValues({a, b, c}, PlaylistColumn::Title).empty());
  EXPECT_EQ(0, PlaylistTagCompletion::FirstPrefixIndex(albums, "du"));
  EXPECT_EQ(1, PlaylistTagCompletion::FirstPrefixIndex(albums, "Thi"));
  EXPECT_EQ(-1, PlaylistTagCompletion::FirstPrefixIndex(albums, "Roads"));
  EXPECT_EQ(-1, PlaylistTagCompletion::FirstPrefixIndex(albums, ""));
}

TEST(PlaylistListLook, PlaybackIconsAndDropRules) {
  EXPECT_EQ(500, PlaylistListLook::kDragHoverTimeoutMs);
  EXPECT_EQ(nullptr, PlaylistListLook::PlaybackIconName(false, PlaylistListLook::Playback::Playing));
  EXPECT_EQ(nullptr, PlaylistListLook::PlaybackIconName(true, PlaylistListLook::Playback::Stopped));
  EXPECT_STREQ("media-playback-start-symbolic", PlaylistListLook::PlaybackIconName(true, PlaylistListLook::Playback::Playing));
  EXPECT_STREQ("media-playback-pause-symbolic", PlaylistListLook::PlaybackIconName(true, PlaylistListLook::Playback::Paused));
  EXPECT_TRUE(PlaylistListLook::IsActiveName("Inbox", "Inbox"));
  EXPECT_FALSE(PlaylistListLook::IsActiveName("Inbox", "Queue"));
  EXPECT_FALSE(PlaylistListLook::IsActiveName("", "Inbox"));

  EXPECT_TRUE(PlaylistListLook::ShouldStartDragHover("strawberry-playlist-rows:0,2"));
  EXPECT_FALSE(PlaylistListLook::ShouldStartDragHover("strawberry-playlist-move:Inbox"));
  EXPECT_FALSE(PlaylistListLook::ShouldStartDragHover("file:///a"));
  EXPECT_TRUE(PlaylistListLook::ShouldRestartDragHover("Queue", "Inbox"));
  EXPECT_FALSE(PlaylistListLook::ShouldRestartDragHover("Inbox", "Inbox"));
  EXPECT_FALSE(PlaylistListLook::ShouldRestartDragHover("", "Inbox"));
  EXPECT_FALSE(PlaylistListLook::DragHoverShouldActivate(499));
  EXPECT_TRUE(PlaylistListLook::DragHoverShouldActivate(500));

  EXPECT_TRUE(PlaylistListLook::ShouldAcceptPlaylistRowsDrop(2, 1));
  EXPECT_FALSE(PlaylistListLook::ShouldAcceptPlaylistRowsDrop(1, 1));
  EXPECT_FALSE(PlaylistListLook::ShouldAcceptPlaylistRowsDrop(-1, 1));
  EXPECT_TRUE(PlaylistListLook::ShouldShowEmptyHint(0));
  EXPECT_FALSE(PlaylistListLook::ShouldShowEmptyHint(1));
  EXPECT_NE(std::string::npos, std::string(PlaylistListLook::EmptyHint()).find("favorite playlists"));
}

TEST(PlaylistListModel, StoresIdsThroughFilter) {
  PlaylistListModel model;
  model.SetRows({"Inbox", "Queue"}, {false, true}, {"", ""}, {4, 7});
  ASSERT_EQ(2u, model.ids().size());
  EXPECT_EQ(4, model.ids()[0]);
  EXPECT_EQ(7, model.ids()[1]);
  PlaylistListSortFilterModel filter(&model);
  const std::vector<PlaylistListDrop::Row> rows = filter.VisibleRows();
  ASSERT_EQ(2u, rows.size());
  EXPECT_EQ("Inbox", rows[0].name);
  EXPECT_EQ(4, rows[0].id);
  EXPECT_EQ("Queue", rows[1].name);
  EXPECT_EQ(7, rows[1].id);
}

TEST(PlaylistTabMenu, ContextActionsMatchQt) {
  const std::vector<PlaylistTabMenu::Item> items = PlaylistTabMenu::Items();
  ASSERT_EQ(6u, items.size());
  EXPECT_STREQ("Star playlist", items[0].label);
  EXPECT_STREQ("Close playlist", items[1].label);
  EXPECT_STREQ("Rename playlist...", items[2].label);
  EXPECT_STREQ("Save playlist...", items[3].label);
  EXPECT_STREQ("New playlist", items[4].label);
  EXPECT_STREQ("Load playlist...", items[5].label);
  EXPECT_TRUE(items[4].separator_before);
  EXPECT_TRUE(items[5].separator_before);
  EXPECT_TRUE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::Star, 0, 2));
  EXPECT_FALSE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::Star, 0, 1));
  EXPECT_FALSE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::Close, -1, 3));
  EXPECT_TRUE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::Rename, 0, 1));
  EXPECT_FALSE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::Save, -1, 2));
  EXPECT_TRUE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::New, -1, 0));
  EXPECT_TRUE(PlaylistTabMenu::ActionEnabled(PlaylistTabMenu::Action::Load, -1, 1));
  EXPECT_EQ(PlaylistTabMenu::Click::Close, PlaylistTabMenu::FromRelease(1, 2));
  EXPECT_EQ(PlaylistTabMenu::Click::None, PlaylistTabMenu::FromRelease(-1, 2));
  EXPECT_EQ(PlaylistTabMenu::Click::None, PlaylistTabMenu::FromRelease(0, 1));
  EXPECT_EQ(PlaylistTabMenu::Click::None, PlaylistTabMenu::FromPress(0, 2, 2));
  EXPECT_EQ(PlaylistTabMenu::Click::Rename, PlaylistTabMenu::FromPress(0, 1, 2));
  EXPECT_EQ(PlaylistTabMenu::Click::New, PlaylistTabMenu::FromPress(-1, 1, 2));
  EXPECT_EQ(PlaylistTabMenu::Click::None, PlaylistTabMenu::FromPress(0, 1, 1));
  EXPECT_TRUE(PlaylistTabMenu::ShouldApplyRename("Inbox", "Shows"));
  EXPECT_FALSE(PlaylistTabMenu::ShouldApplyRename("Inbox", "Inbox"));
  EXPECT_FALSE(PlaylistTabMenu::ShouldApplyRename("Inbox", ""));
  EXPECT_TRUE(PlaylistTabMenu::CloseCurrentHidesWindow(1));
  EXPECT_FALSE(PlaylistTabMenu::CloseCurrentHidesWindow(2));
  EXPECT_TRUE(PlaylistTabMenu::ToggledFavorite(false));
  EXPECT_FALSE(PlaylistTabMenu::ToggledFavorite(true));
  EXPECT_STREQ(FavoriteWidget::TooltipText(), PlaylistTabMenu::FavoriteTooltip());
  EXPECT_EQ("strawberry-playlist-tab:7", PlaylistTabMenu::TabPayload(7));
  EXPECT_TRUE(PlaylistTabMenu::IsTabPayload("strawberry-playlist-tab:7"));
  EXPECT_EQ(7, PlaylistTabMenu::ParseTabId("strawberry-playlist-tab:7"));
  EXPECT_EQ(-1, PlaylistTabMenu::ParseTabId("strawberry-playlist-rows:1"));
  EXPECT_TRUE(PlaylistTabMenu::ShouldHoverForPayload("strawberry-playlist-rows:1,2"));
  EXPECT_TRUE(PlaylistTabMenu::ShouldHoverForPayload("file:///tmp/a.mp3"));
  EXPECT_FALSE(PlaylistTabMenu::ShouldHoverForPayload("strawberry-playlist-tab:3"));
  EXPECT_FALSE(PlaylistTabMenu::ShouldHoverForPayload(""));
  EXPECT_TRUE(PlaylistTabMenu::DropOnEmptyCreatesPlaylist());
  EXPECT_EQ(500, PlaylistTabMenu::kDragHoverTimeoutMs);
  const std::vector<int> moved = PlaylistTabMenu::ReorderIds({1, 2, 3}, 0, 2);
  ASSERT_EQ(3u, moved.size());
  EXPECT_EQ(2, moved[0]);
  EXPECT_EQ(1, moved[1]);
  EXPECT_EQ(3, moved[2]);
  const std::vector<int> last = PlaylistTabMenu::ReorderIds({1, 2, 3}, 2, 0);
  ASSERT_EQ(3u, last.size());
  EXPECT_EQ(3, last[0]);
  EXPECT_EQ(1, last[1]);
  EXPECT_EQ(2, last[2]);
}

TEST(PlaylistTabNavigation, WrapsLikeQtShortcuts) {
  EXPECT_EQ(-1, PlaylistTabNavigation::NextIndex(0, 0));
  EXPECT_EQ(1, PlaylistTabNavigation::NextIndex(0, 3));
  EXPECT_EQ(0, PlaylistTabNavigation::NextIndex(2, 3));
  EXPECT_EQ(0, PlaylistTabNavigation::NextIndex(-1, 3));
  EXPECT_EQ(1, PlaylistTabNavigation::PreviousIndex(2, 3));
  EXPECT_EQ(2, PlaylistTabNavigation::PreviousIndex(0, 3));
  EXPECT_EQ(2, PlaylistTabNavigation::PreviousIndex(-1, 3));
  EXPECT_EQ(2, PlaylistTabNavigation::LastIndex(3));
  EXPECT_EQ(-1, PlaylistTabNavigation::LastIndex(0));
  EXPECT_EQ(1, PlaylistTabNavigation::ActiveIndex(1, 3));
  EXPECT_EQ(-1, PlaylistTabNavigation::ActiveIndex(5, 3));
  EXPECT_EQ(-1, PlaylistTabNavigation::ActiveIndex(-1, 3));
}

TEST(PlaylistHeaderSort, MarksAndMenuChecksMatchQtHeader) {
  EXPECT_FALSE(PlaylistHeaderSort::IsSorted(PlaylistColumn::Title, PlaylistColumn::Count));
  EXPECT_TRUE(PlaylistHeaderSort::IsSorted(PlaylistColumn::Title, PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistHeaderSort::IsSorted(PlaylistColumn::Artist, PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistHeaderSort::HasSort(PlaylistColumn::Count));
  EXPECT_TRUE(PlaylistHeaderSort::HasSort(PlaylistColumn::Title));
  EXPECT_EQ("Title", PlaylistHeaderSort::Label("Title", false, false));
  EXPECT_EQ(std::string("Title") + PlaylistHeaderSort::kAscendingMark, PlaylistHeaderSort::Label("Title", true, false));
  EXPECT_EQ(std::string("Artist") + PlaylistHeaderSort::kDescendingMark, PlaylistHeaderSort::Label("Artist", true, true));
  EXPECT_EQ(std::string("Title") + PlaylistHeaderSort::kAscendingMark,
            PlaylistHeaderSort::LabelForColumn("Title", PlaylistColumn::Title, PlaylistColumn::Title, false));
  EXPECT_EQ("Album", PlaylistHeaderSort::LabelForColumn("Album", PlaylistColumn::Album, PlaylistColumn::Title, false));
  EXPECT_TRUE(PlaylistHeaderSort::AscendingChecked(PlaylistColumn::Title, PlaylistColumn::Title, false));
  EXPECT_FALSE(PlaylistHeaderSort::DescendingChecked(PlaylistColumn::Title, PlaylistColumn::Title, false));
  EXPECT_TRUE(PlaylistHeaderSort::DescendingChecked(PlaylistColumn::Year, PlaylistColumn::Year, true));
  EXPECT_FALSE(PlaylistHeaderSort::AscendingChecked(PlaylistColumn::Year, PlaylistColumn::Title, false));
  EXPECT_TRUE(PlaylistHeaderSort::ClearEnabled(PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistHeaderSort::ClearEnabled(PlaylistColumn::Count));
}

TEST(PlaylistHeaderSort, ApplyOrderAndSkipRedundantExplicitSort) {
  const PlaylistHeaderSort::State unsorted;
  const PlaylistHeaderSort::State title_asc = PlaylistHeaderSort::ApplyOrder(unsorted, PlaylistColumn::Title, PlaylistSortOrder::Toggle);
  EXPECT_EQ(PlaylistColumn::Title, title_asc.column);
  EXPECT_FALSE(title_asc.descending);
  const PlaylistHeaderSort::State title_desc = PlaylistHeaderSort::ApplyOrder(title_asc, PlaylistColumn::Title, PlaylistSortOrder::Toggle);
  EXPECT_TRUE(title_desc.descending);
  const PlaylistHeaderSort::State title_cleared = PlaylistHeaderSort::ApplyOrder(title_desc, PlaylistColumn::Title, PlaylistSortOrder::Toggle);
  EXPECT_EQ(PlaylistColumn::Count, title_cleared.column);
  EXPECT_FALSE(title_cleared.descending);
  const PlaylistHeaderSort::State artist = PlaylistHeaderSort::ApplyOrder(title_desc, PlaylistColumn::Artist, PlaylistSortOrder::Toggle);
  EXPECT_EQ(PlaylistColumn::Artist, artist.column);
  EXPECT_FALSE(artist.descending);
  const PlaylistHeaderSort::State explicit_desc =
      PlaylistHeaderSort::ApplyOrder(title_asc, PlaylistColumn::Album, PlaylistSortOrder::Descending);
  EXPECT_EQ(PlaylistColumn::Album, explicit_desc.column);
  EXPECT_TRUE(explicit_desc.descending);
  const PlaylistHeaderSort::State cleared = PlaylistHeaderSort::ApplyOrder(explicit_desc, PlaylistColumn::Album, PlaylistSortOrder::Clear);
  EXPECT_EQ(PlaylistColumn::Count, cleared.column);
  EXPECT_FALSE(cleared.descending);
  EXPECT_FALSE(PlaylistHeaderSort::ShouldApplyExplicit(PlaylistColumn::Title, PlaylistColumn::Title, false, PlaylistSortOrder::Ascending));
  EXPECT_TRUE(PlaylistHeaderSort::ShouldApplyExplicit(PlaylistColumn::Title, PlaylistColumn::Title, false, PlaylistSortOrder::Descending));
  EXPECT_TRUE(PlaylistHeaderSort::ShouldApplyExplicit(PlaylistColumn::Title, PlaylistColumn::Title, false, PlaylistSortOrder::Clear));
  EXPECT_FALSE(PlaylistHeaderSort::ShouldApplyExplicit(PlaylistColumn::Title, PlaylistColumn::Count, false, PlaylistSortOrder::Clear));
  EXPECT_TRUE(PlaylistHeaderSort::ShouldApplyExplicit(PlaylistColumn::Title, PlaylistColumn::Artist, true, PlaylistSortOrder::Toggle));
  EXPECT_TRUE(PlaylistHeaderSort::ShouldSortNow(PlaylistColumn::Title));
  EXPECT_FALSE(PlaylistHeaderSort::ShouldSortNow(PlaylistColumn::Count));
}

TEST(PlaylistColumnWidths, QtDefaultsNormalizeAndNeighborResize) {
  EXPECT_EQ(2, PlaylistColumnWidths::kHeaderStateVersion);
  EXPECT_EQ(30, PlaylistColumnWidths::kMinSectionSize);
  EXPECT_DOUBLE_EQ(0.06, PlaylistColumnWidths::DefaultProportion(PlaylistColumn::Track));
  EXPECT_DOUBLE_EQ(0.23, PlaylistColumnWidths::DefaultProportion(PlaylistColumn::Title));
  EXPECT_DOUBLE_EQ(0.23, PlaylistColumnWidths::DefaultProportion(PlaylistColumn::Artist));
  EXPECT_DOUBLE_EQ(0.23, PlaylistColumnWidths::DefaultProportion(PlaylistColumn::Album));
  EXPECT_DOUBLE_EQ(0.05, PlaylistColumnWidths::DefaultProportion(PlaylistColumn::Samplerate));
  EXPECT_DOUBLE_EQ(0.04, PlaylistColumnWidths::DefaultProportion(PlaylistColumn::Length));
  EXPECT_TRUE(PlaylistColumnWidths::VersionSupported(2));
  EXPECT_FALSE(PlaylistColumnWidths::VersionSupported(1));
  const auto normalized = PlaylistColumnWidths::Normalize({0.23, 0.23});
  ASSERT_EQ(2u, normalized.size());
  EXPECT_NEAR(0.5, normalized[0], 0.0001);
  EXPECT_NEAR(0.5, normalized[1], 0.0001);
  const auto empty = PlaylistColumnWidths::Normalize({0.0, 0.0});
  ASSERT_EQ(2u, empty.size());
  EXPECT_NEAR(0.5, empty[0], 0.0001);
  const auto pixels = PlaylistColumnWidths::Distribute({0.25, 0.75}, 400);
  ASSERT_EQ(2u, pixels.size());
  EXPECT_EQ(100, pixels[0]);
  EXPECT_EQ(300, pixels[1]);
  int right = 0;
  EXPECT_TRUE(PlaylistColumnWidths::NeighborResize(100, 140, 200, &right));
  EXPECT_EQ(160, right);
  EXPECT_FALSE(PlaylistColumnWidths::NeighborResize(100, 280, 200, &right));
  EXPECT_TRUE(PlaylistColumnWidths::OnResizeHandle(96, 100));
  EXPECT_FALSE(PlaylistColumnWidths::OnResizeHandle(40, 100));
  EXPECT_TRUE(PlaylistColumnWidths::OnResizeHandleAbsolute(204, 100, 100));
}

TEST(PlaylistColumnWidths, EncodeDecodeRoundTripAndRejectQtBlob) {
  PlaylistColumnWidths::State state;
  state.stretch = true;
  state.proportions[PlaylistColumn::Title] = 0.23;
  state.proportions[PlaylistColumn::Artist] = 0.23;
  state.pixels[PlaylistColumn::Title] = 184;
  const std::string blob = PlaylistColumnWidths::Encode(state);
  const PlaylistColumnWidths::State loaded = PlaylistColumnWidths::Decode(blob);
  EXPECT_EQ(PlaylistColumnWidths::kHeaderStateVersion, loaded.version);
  EXPECT_TRUE(loaded.stretch);
  ASSERT_EQ(1u, loaded.proportions.count(PlaylistColumn::Title));
  ASSERT_EQ(1u, loaded.proportions.count(PlaylistColumn::Artist));
  EXPECT_NEAR(0.23, loaded.proportions.at(PlaylistColumn::Title), 0.0001);
  EXPECT_NEAR(0.23, loaded.proportions.at(PlaylistColumn::Artist), 0.0001);
  ASSERT_EQ(1u, loaded.pixels.count(PlaylistColumn::Title));
  EXPECT_EQ(184, loaded.pixels.at(PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistColumnWidths::Decode({}).proportions.empty());
  EXPECT_TRUE(PlaylistColumnWidths::Decode("not-a-header-state").proportions.empty());
  EXPECT_TRUE(PlaylistColumnWidths::Decode("1;1;Title=0.23;").proportions.empty());
}

TEST(PlaylistColumnLayout, PixelWidthsFollowSavedProportions) {
  PlaylistColumnLayout::Reset();
  PlaylistColumnLayout::SetVisibleColumns({PlaylistColumn::Title, PlaylistColumn::Artist});
  EXPECT_TRUE(PlaylistColumnLayout::StretchEnabled());
  const auto stretched = PlaylistColumnLayout::PixelWidths(1000);
  ASSERT_EQ(2u, stretched.size());
  EXPECT_EQ(500, stretched[0]);
  EXPECT_EQ(500, stretched[1]);
  EXPECT_EQ(500, PlaylistColumnLayout::PixelWidth(PlaylistColumn::Title, 1000));
  PlaylistColumnLayout::ResizePair(PlaylistColumn::Title, 600, PlaylistColumn::Artist, 400, 1000);
  const auto resized = PlaylistColumnLayout::PixelWidths(1000);
  ASSERT_EQ(2u, resized.size());
  EXPECT_EQ(600, resized[0]);
  EXPECT_EQ(400, resized[1]);
  PlaylistColumnLayout::SetStretchEnabled(false);
  PlaylistColumnLayout::ResizePair(PlaylistColumn::Title, 220, PlaylistColumn::Artist, 180, 0);
  EXPECT_EQ(220, PlaylistColumnLayout::PixelWidth(PlaylistColumn::Title, 800));
  EXPECT_EQ(180, PlaylistColumnLayout::PixelWidth(PlaylistColumn::Artist, 800));
  PlaylistColumnLayout::Reset();
}

TEST(PlaylistAutoscroll, MaybeAlwaysNeverAndCenter) {
  EXPECT_EQ(30000, PlaylistAutoscroll::kGraceMs);
  EXPECT_TRUE(PlaylistAutoscroll::ShouldScroll(Playlist::AutoScroll::Always, true));
  EXPECT_FALSE(PlaylistAutoscroll::ShouldScroll(Playlist::AutoScroll::Never, false));
  EXPECT_TRUE(PlaylistAutoscroll::ShouldScroll(Playlist::AutoScroll::Maybe, false));
  EXPECT_FALSE(PlaylistAutoscroll::ShouldScroll(Playlist::AutoScroll::Maybe, true));
  EXPECT_TRUE(PlaylistAutoscroll::ShouldSkipIfVisible(true));
  EXPECT_FALSE(PlaylistAutoscroll::ShouldSkipIfVisible(false));
  EXPECT_EQ(80, PlaylistAutoscroll::CenteredOffset(200, 40, 280));
}

TEST(PlaylistClipboard, JoinsVisibleColumnsAndCopyShortcut) {
  EXPECT_EQ("Roads - Portishead", PlaylistClipboard::DisplayText({"Roads", "", "Portishead"}));
  EXPECT_TRUE(PlaylistClipboard::DisplayText({}).empty());
  EXPECT_TRUE(PlaylistClipboard::IsCopyShortcut('c', 4, 4));
  EXPECT_TRUE(PlaylistClipboard::IsCopyShortcut('C', 4, 4));
  EXPECT_FALSE(PlaylistClipboard::IsCopyShortcut('c', 0, 4));
  EXPECT_FALSE(PlaylistClipboard::IsCopyShortcut('x', 4, 4));
}

TEST(PlaylistHeaderReorder, ModeAndOrderAfterMove) {
  EXPECT_EQ(PlaylistHeaderReorder::DragMode::Resize, PlaylistHeaderReorder::ModeAt(true, PlaylistColumn::Title));
  EXPECT_EQ(PlaylistHeaderReorder::DragMode::Reorder, PlaylistHeaderReorder::ModeAt(false, PlaylistColumn::Title));
  EXPECT_EQ(PlaylistHeaderReorder::DragMode::None, PlaylistHeaderReorder::ModeAt(false, PlaylistColumn::Count));
  const std::vector<PlaylistColumn> visible = {PlaylistColumn::Title, PlaylistColumn::Artist, PlaylistColumn::Album};
  EXPECT_EQ(1, PlaylistHeaderReorder::VisualIndex(visible, PlaylistColumn::Artist));
  EXPECT_EQ(-1, PlaylistHeaderReorder::VisualIndex(visible, PlaylistColumn::Year));
  const auto moved = PlaylistHeaderReorder::OrderAfterMove(visible, PlaylistColumn::Title, 2);
  ASSERT_EQ(3u, moved.size());
  EXPECT_EQ(PlaylistColumn::Artist, moved[0]);
  EXPECT_EQ(PlaylistColumn::Title, moved[1]);
  EXPECT_EQ(PlaylistColumn::Album, moved[2]);
  const auto left = PlaylistHeaderReorder::OrderAfterMove(visible, PlaylistColumn::Album, 0);
  ASSERT_EQ(3u, left.size());
  EXPECT_EQ(PlaylistColumn::Album, left[0]);
  EXPECT_EQ(PlaylistColumn::Title, left[1]);
  EXPECT_EQ(PlaylistColumn::Artist, left[2]);
  EXPECT_TRUE(PlaylistHeaderReorder::ShouldApplyReorder(PlaylistColumn::Title, PlaylistColumn::Artist));
  EXPECT_FALSE(PlaylistHeaderReorder::ShouldApplyReorder(PlaylistColumn::Title, PlaylistColumn::Title));
  EXPECT_TRUE(PlaylistHeaderReorder::ShouldStartReorder(40, 0, 100));
  EXPECT_FALSE(PlaylistHeaderReorder::ShouldStartReorder(96, 0, 100));
}

TEST(PlaylistDropIndicator, MidpointInsertAndEmpty) {
  EXPECT_EQ(2, PlaylistDropIndicator::kLineWidth);
  EXPECT_EQ(5, PlaylistDropIndicator::kGradientWidth);
  const auto empty = PlaylistDropIndicator::FromPointer(10, -1, 0, 0, false, 0);
  EXPECT_EQ(0, empty.insert_row);
  EXPECT_EQ(PlaylistDropIndicator::Position::Empty, empty.pos);
  EXPECT_EQ(1, empty.line_y);
  const auto above = PlaylistDropIndicator::FromPointer(10, 3, 0, 40, true, 40);
  EXPECT_EQ(3, above.insert_row);
  EXPECT_EQ(PlaylistDropIndicator::Position::Above, above.pos);
  EXPECT_EQ(0, above.line_y);
  const auto below = PlaylistDropIndicator::FromPointer(30, 3, 0, 40, true, 40);
  EXPECT_EQ(4, below.insert_row);
  EXPECT_EQ(PlaylistDropIndicator::Position::Below, below.pos);
  EXPECT_EQ(40, below.line_y);
  EXPECT_TRUE(PlaylistDropIndicator::Active(above));
  EXPECT_EQ(4, PlaylistDropIndicator::InsertRow(below, 0));
  EXPECT_EQ(7, PlaylistDropIndicator::InsertRow({}, 7));
}

TEST(PlaylistKeyboard, SpacePlayPauseAndArrowsSeek) {
  EXPECT_EQ(PlaylistKeyboard::Action::PlayPause, PlaylistKeyboard::FromKey(PlaylistKeyboard::kSpace, 0));
  EXPECT_EQ(PlaylistKeyboard::Action::None, PlaylistKeyboard::FromKey(PlaylistKeyboard::kSpace, PlaylistKeyboard::kControlMask));
  EXPECT_EQ(PlaylistKeyboard::Action::SeekBack, PlaylistKeyboard::FromKey(PlaylistKeyboard::kLeft, 0));
  EXPECT_EQ(PlaylistKeyboard::Action::SeekForward, PlaylistKeyboard::FromKey(PlaylistKeyboard::kRight, 0));
  EXPECT_EQ(PlaylistKeyboard::Action::None, PlaylistKeyboard::FromKey('c', 0));
}

TEST(PlaylistPlayingIcon, PauseAndPlayNames) {
  EXPECT_STREQ("media-playback-pause-symbolic", PlaylistPlayingIcon::Name(true));
  EXPECT_STREQ("media-playback-start-symbolic", PlaylistPlayingIcon::Name(false));
  EXPECT_TRUE(PlaylistPlayingIcon::ShowOnCurrentRow(true));
  EXPECT_FALSE(PlaylistPlayingIcon::ShowOnCurrentRow(false));
  EXPECT_EQ(12, PlaylistPlayingIcon::kPixelSize);
}
