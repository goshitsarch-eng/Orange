#include "playlist/playlistmanager.h"
#include "queue/queue.h"

#include "utilities/fileutils.h"

#include <gtest/gtest.h>

namespace {

Song MakeSong(const std::string &title, const std::string &url) {
  Song song;
  song.set_title(title);
  song.set_url(url);
  song.set_valid(true);
  return song;
}

}  // namespace

TEST(PlaylistManager, InitCreatesDefaultPlaylist) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  ASSERT_NE(nullptr, manager.current());
  EXPECT_EQ(manager.current(), manager.active());
  EXPECT_FALSE(manager.playlist_ids().empty());
  EXPECT_EQ("Playlist", manager.playlist_name(manager.current_id()));
}

TEST(PlaylistManager, NewLoadSaveRenameCloseAndCurrentActive) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  const int first = manager.current_id();
  Playlist *created = manager.New("Second", {MakeSong("One", "file:///one")});
  ASSERT_NE(nullptr, created);
  EXPECT_EQ("Second", created->name());
  EXPECT_EQ(1, created->row_count());
  EXPECT_EQ(created, manager.current());
  EXPECT_EQ(first, manager.active_id());

  manager.SetActiveToCurrent();
  EXPECT_EQ(manager.current_id(), manager.active_id());

  manager.SetCurrentPlaylist(first);
  EXPECT_EQ(first, manager.current_id());
  EXPECT_NE(first, manager.active_id());

  manager.Rename(manager.active_id(), "Playing");
  EXPECT_EQ("Playing", manager.playlist_name(manager.active_id()));

  const std::string path = "/tmp/strawberry-playlistmanager-test.m3u";
  manager.Save(manager.active_id(), path);
  ASSERT_TRUE(FileUtils::Exists(path));
  manager.Load(path);
  EXPECT_EQ("strawberry-playlistmanager-test", manager.current()->name());
  EXPECT_GE(manager.current()->row_count(), 1);

  const int loaded = manager.current_id();
  EXPECT_TRUE(manager.Close(loaded));
  EXPECT_EQ(nullptr, manager.playlist(loaded));
  FileUtils::Remove(path);
}

TEST(PlaylistManager, PlaylistMutationsAndSmartPlaylist) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  manager.AppendSongs({MakeSong("A", "file:///a"), MakeSong("A-dup", "file:///a"), MakeSong("B", "file:///b")});
  EXPECT_EQ(3, manager.current()->row_count());
  manager.RemoveDuplicatesCurrent();
  EXPECT_EQ(2, manager.current()->row_count());
  manager.ShuffleCurrent();
  EXPECT_EQ(2, manager.current()->row_count());
  manager.current()->set_current_row(0);
  manager.RateCurrentSong2(4);
  EXPECT_NEAR(0.8f, manager.current()->current_song().rating(), 0.001f);
  manager.ClearCurrent();
  EXPECT_EQ(0, manager.current()->row_count());
  manager.PlaySmartPlaylist("all", true, true);
  EXPECT_EQ("All songs", manager.current()->name());
  EXPECT_TRUE(manager.current()->is_dynamic());
}

TEST(Queue, ContainsAndRemoveSong) {
  Queue queue;
  Song a;
  a.set_url("file:///a");
  a.set_title("A");
  Song b;
  b.set_url("file:///b");
  b.set_title("B");
  queue.Append(a);
  queue.InsertNext(b);
  EXPECT_TRUE(queue.Contains(a));
  EXPECT_EQ(2, queue.size());
  queue.RemoveSong(a);
  EXPECT_FALSE(queue.Contains(a));
  EXPECT_TRUE(queue.Contains(b));
}

TEST(PlaylistManager, ChangeOrderAndFavorite) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  const int first = manager.current_id();
  manager.New("Two");
  const int second = manager.current_id();
  manager.ChangePlaylistOrder({second, first});
  ASSERT_EQ(2u, manager.playlist_ids().size());
  EXPECT_EQ(second, manager.playlist_ids().front());
  manager.Favorite(second, true);
  EXPECT_TRUE(manager.playlist(second)->favorite());
  manager.Delete(second);
  EXPECT_EQ(nullptr, manager.playlist(second));
  EXPECT_FALSE(manager.GetAllPlaylists().empty());
}
