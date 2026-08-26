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
