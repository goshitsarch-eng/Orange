#include "core/song.h"
#include "playlist/playlistdropindicator.h"
#include "queue/queuekeyboard.h"
#include "queue/queueui.h"

#include <gtest/gtest.h>

TEST(QueueUi, SummaryTextMatchesQt) {
  EXPECT_EQ("0 tracks", QueueUi::SummaryText(0, 0));
  EXPECT_EQ("1 track", QueueUi::SummaryText(1, 0));
  EXPECT_EQ("2 tracks", QueueUi::SummaryText(2, 0));
  EXPECT_EQ("1 track - [ 3 minutes ]", QueueUi::SummaryText(1, 180000000000LL));
  EXPECT_EQ("2 tracks - [ 1 hour 1 minute ]", QueueUi::SummaryText(2, 3660000000000LL));

  Song short_song;
  short_song.set_length_nanosec(60000000000LL);
  Song long_song;
  long_song.set_length_nanosec(120000000000LL);
  EXPECT_EQ(180000000000LL, QueueUi::TotalLengthNanosec({short_song, long_song}));
  EXPECT_EQ("2 tracks - [ 3 minutes ]", QueueUi::SummaryText({short_song, long_song}));
}

TEST(QueueUi, ButtonStateMatchesQt) {
  const QueueUi::ButtonState empty = QueueUi::Buttons({}, 0);
  EXPECT_FALSE(empty.clear);
  EXPECT_FALSE(empty.remove);
  EXPECT_FALSE(empty.move_up);
  EXPECT_FALSE(empty.move_down);

  const QueueUi::ButtonState none = QueueUi::Buttons({}, 3);
  EXPECT_TRUE(none.clear);
  EXPECT_FALSE(none.remove);
  EXPECT_FALSE(none.move_up);
  EXPECT_FALSE(none.move_down);

  const QueueUi::ButtonState mid = QueueUi::Buttons({1}, 3);
  EXPECT_TRUE(mid.clear);
  EXPECT_TRUE(mid.remove);
  EXPECT_TRUE(mid.move_up);
  EXPECT_TRUE(mid.move_down);

  const QueueUi::ButtonState first = QueueUi::Buttons({0}, 3);
  EXPECT_FALSE(first.move_up);
  EXPECT_TRUE(first.move_down);

  const QueueUi::ButtonState last = QueueUi::Buttons({2}, 3);
  EXPECT_TRUE(last.move_up);
  EXPECT_FALSE(last.move_down);

  const QueueUi::ButtonState all = QueueUi::Buttons({0, 1, 2}, 3);
  EXPECT_TRUE(all.remove);
  EXPECT_FALSE(all.move_up);
  EXPECT_FALSE(all.move_down);

  const QueueUi::ButtonState ends = QueueUi::Buttons({0, 2}, 3);
  EXPECT_FALSE(ends.move_up);
  EXPECT_FALSE(ends.move_down);
}

TEST(QueueUi, ToolbuttonIconsMatchQt) {
  EXPECT_STREQ("go-down-symbolic", QueueUi::MoveDownIcon());
  EXPECT_STREQ("go-up-symbolic", QueueUi::MoveUpIcon());
  EXPECT_STREQ("list-remove-symbolic", QueueUi::RemoveIcon());
  EXPECT_STREQ("edit-clear-symbolic", QueueUi::ClearIcon());
  EXPECT_STREQ("Move down", QueueUi::MoveDownTooltip());
  EXPECT_STREQ("Move up", QueueUi::MoveUpTooltip());
  EXPECT_STREQ("Remove", QueueUi::RemoveTooltip());
  EXPECT_STREQ("Clear", QueueUi::ClearTooltip());
}

TEST(QueueKeyboard, MatchesQtShortcuts) {
  EXPECT_EQ(QueueKeyboard::Action::Remove, QueueKeyboard::FromKey(QueueKeyboard::kDelete, 0));
  EXPECT_EQ(QueueKeyboard::Action::Clear, QueueKeyboard::FromKey('k', QueueKeyboard::kControlMask));
  EXPECT_EQ(QueueKeyboard::Action::Clear, QueueKeyboard::FromKey('K', QueueKeyboard::kControlMask));
  EXPECT_EQ(QueueKeyboard::Action::None, QueueKeyboard::FromKey('k', 0));
  EXPECT_EQ(QueueKeyboard::Action::MoveDown, QueueKeyboard::FromKey(QueueKeyboard::kUp, QueueKeyboard::kControlMask));
  EXPECT_EQ(QueueKeyboard::Action::MoveUp, QueueKeyboard::FromKey(QueueKeyboard::kDown, QueueKeyboard::kControlMask));
  EXPECT_EQ(QueueKeyboard::Action::None, QueueKeyboard::FromKey(QueueKeyboard::kUp, 0));
  EXPECT_EQ(QueueKeyboard::Action::None, QueueKeyboard::FromKey(QueueKeyboard::kDown, 0));
}

TEST(QueueDropIndicator, ReusesPlaylistMidpointInsert) {
  const auto empty = PlaylistDropIndicator::FromPointer(10, -1, 0, 0, false, 0);
  EXPECT_EQ(0, empty.insert_row);
  EXPECT_EQ(PlaylistDropIndicator::Position::Empty, empty.pos);
  EXPECT_EQ(1, empty.line_y);
  const auto above = PlaylistDropIndicator::FromPointer(10, 1, 0, 40, true, 40);
  EXPECT_EQ(1, PlaylistDropIndicator::InsertRow(above, 0));
  const auto below = PlaylistDropIndicator::FromPointer(30, 1, 0, 40, true, 40);
  EXPECT_EQ(2, PlaylistDropIndicator::InsertRow(below, 0));
}
