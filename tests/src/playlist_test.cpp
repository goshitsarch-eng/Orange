#include "playlist/playlist.h"

#include <gtest/gtest.h>

TEST(Playlist, AppendAndNavigate) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_url("file:///b");
  b.set_valid(true);
  playlist.AppendSongs({a, b});
  EXPECT_EQ(2, playlist.row_count());
  playlist.set_current_row(0);
  playlist.Next();
  EXPECT_EQ("B", playlist.current_song().title());
  playlist.Clear();
  EXPECT_EQ(0, playlist.row_count());
}

TEST(Playlist, DynamicRefill) {
  Playlist playlist;
  SmartPlaylistSearch search;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "A"});
  playlist.SetDynamic(true, search);
  EXPECT_TRUE(playlist.is_dynamic());
  Song have;
  have.set_title("Have");
  have.set_artist("A");
  have.set_url("file:///have");
  have.set_valid(true);
  playlist.AppendSongs({have});
  Song extra;
  extra.set_title("Extra");
  extra.set_artist("A Band");
  extra.set_url("file:///extra");
  extra.set_valid(true);
  Song skip;
  skip.set_title("Skip");
  skip.set_artist("Other");
  skip.set_url("file:///skip");
  skip.set_valid(true);
  playlist.RefillDynamic({have, extra, skip});
  EXPECT_EQ(2, playlist.row_count());
  EXPECT_EQ("Extra", playlist.songs().back().title());
}

TEST(Playlist, PeekNextDoesNotAdvance) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_url("file:///b");
  b.set_valid(true);
  playlist.AppendSongs({a, b});
  playlist.set_current_row(0);
  EXPECT_EQ(1, playlist.PeekNextRow());
  EXPECT_EQ("B", playlist.PeekNextSong().title());
  EXPECT_EQ(0, playlist.current_row());
  playlist.SetSequenceMode(Playlist::SequenceMode::RepeatAll);
  playlist.set_current_row(1);
  EXPECT_EQ(0, playlist.PeekNextRow());
  EXPECT_EQ("A", playlist.PeekNextSong().title());
}

TEST(Playlist, UndoRedo) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_url("file:///b");
  b.set_valid(true);
  playlist.AppendSongs({a});
  playlist.AppendSongs({b});
  EXPECT_EQ(2, playlist.row_count());
  EXPECT_TRUE(playlist.CanUndo());
  playlist.Undo();
  EXPECT_EQ(1, playlist.row_count());
  EXPECT_EQ("A", playlist.songs().front().title());
  EXPECT_TRUE(playlist.CanRedo());
  playlist.Redo();
  EXPECT_EQ(2, playlist.row_count());
  playlist.Clear();
  EXPECT_EQ(0, playlist.row_count());
  playlist.Undo();
  EXPECT_EQ(2, playlist.row_count());
}
