#include "playlist/playlist.h"
#include "utilities/fileutils.h"

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

TEST(Playlist, RemoveDuplicatesKeepsFirstUrl) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_url("file:///same");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_url("file:///same");
  b.set_valid(true);
  Song c;
  c.set_title("C");
  c.set_url("file:///other");
  c.set_valid(true);
  playlist.AppendSongs({a, b, c});
  playlist.RemoveDuplicates();
  EXPECT_EQ(2, playlist.row_count());
  EXPECT_EQ("A", playlist.songs().front().title());
  EXPECT_EQ("C", playlist.songs().back().title());
}

TEST(Playlist, RemoveUnavailableDropsMissingLocalFiles) {
  Playlist playlist;
  const std::string existing = "/tmp/strawberry-playlist-exists.txt";
  FileUtils::WriteFile(existing, "ok");
  Song keep;
  keep.set_title("Keep");
  keep.set_url(FileUtils::UriFromPath(existing));
  keep.set_valid(true);
  Song gone;
  gone.set_title("Gone");
  gone.set_url("file:///tmp/strawberry-playlist-missing-file.mp3");
  gone.set_valid(true);
  Song stream;
  stream.set_title("Stream");
  stream.set_url("http://example.invalid/live");
  stream.set_valid(true);
  playlist.AppendSongs({keep, gone, stream});
  playlist.RemoveUnavailable();
  EXPECT_EQ(2, playlist.row_count());
  EXPECT_EQ("Keep", playlist.songs().front().title());
  EXPECT_EQ("Stream", playlist.songs().back().title());
  FileUtils::Remove(existing);
}

TEST(Playlist, SkipTracksAreBypassedOnNext) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_url("file:///b");
  b.set_valid(true);
  Song c;
  c.set_title("C");
  c.set_url("file:///c");
  c.set_valid(true);
  playlist.AppendSongs({a, b, c});
  playlist.set_current_row(0);
  playlist.SkipTracks({1});
  EXPECT_TRUE(playlist.songs()[1].skipped());
  EXPECT_EQ(2, playlist.PeekNextRow());
  playlist.Next();
  EXPECT_EQ("C", playlist.current_song().title());
  playlist.SkipTracks({1});
  EXPECT_FALSE(playlist.songs()[1].skipped());
}

TEST(Playlist, RenumberAndRateCurrent) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_url("file:///a");
  a.set_track(9);
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_url("file:///b");
  b.set_track(3);
  b.set_valid(true);
  playlist.AppendSongs({a, b});
  playlist.RenumberTracks();
  EXPECT_EQ(1, playlist.songs()[0].track());
  EXPECT_EQ(2, playlist.songs()[1].track());
  playlist.set_current_row(0);
  playlist.RateCurrentSong(0.8f);
  EXPECT_NEAR(0.8f, playlist.current_song().rating(), 0.001f);
}
