#include "playlist/playlistcollectionsync.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlistbackend.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistdynamicpersist.h"
#include "playlist/playlistitemuuid.h"
#include "playlist/playlistdragpayload.h"
#include "playlist/playlistcrossundopair.h"
#include "playlist/playlistlistdrop.h"
#include "playlist/playlistqueuescope.h"
#include "core/database.h"
#include "tagreader/tagreader.h"
#include "queue/queue.h"
#include "queue/queuedrop.h"
#include "queue/queuerows.h"

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
  const int smart_id = manager.current_id();
  manager.New("Scratch");
  manager.AppendSongs({MakeSong("Keep", "file:///keep")});
  EXPECT_EQ(1, manager.current()->row_count());
  manager.PlaySmartPlaylist("all", false, false);
  EXPECT_EQ("Scratch", manager.current()->name());
  EXPECT_GE(manager.current()->row_count(), 1);
  EXPECT_FALSE(manager.current()->is_dynamic());
  manager.PlaySmartPlaylist("never", false, true);
  EXPECT_EQ("Scratch", manager.current()->name());
  EXPECT_TRUE(manager.current()->is_dynamic());
  EXPECT_NE(smart_id, manager.current_id());
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

TEST(Queue, InsertAndMoveRows) {
  Queue queue;
  queue.Append(MakeSong("A", "file:///a"));
  queue.Append(MakeSong("B", "file:///b"));
  queue.Insert(1, {MakeSong("C", "file:///c")});
  ASSERT_EQ(3, queue.size());
  EXPECT_EQ("file:///c", queue.songs()[1].url());
  queue.MoveRows({0, 1}, 3);
  ASSERT_EQ(3, queue.size());
  EXPECT_EQ("file:///b", queue.songs()[0].url());
  EXPECT_EQ("file:///a", queue.songs()[1].url());
  EXPECT_EQ("file:///c", queue.songs()[2].url());
  queue.RemoveRows({0, 2});
  ASSERT_EQ(1, queue.size());
  EXPECT_EQ("file:///a", queue.songs()[0].url());
}

TEST(QueueDrop, ParsesRowsAndUrls) {
  EXPECT_TRUE(QueueDrop::IsQueueRows("strawberry-queue-rows:1,2"));
  EXPECT_TRUE(QueueDrop::IsPlaylistRows("strawberry-playlist-rows:0,3"));
  EXPECT_FALSE(QueueDrop::IsQueueRows("https://example.com/a\nhttps://example.com/b"));
  const std::vector<int> rows = QueueDrop::ParseRows("strawberry-queue-rows:1,4,2", QueueDrop::kQueueRowsPrefix);
  ASSERT_EQ(3u, rows.size());
  EXPECT_EQ(1, rows[0]);
  EXPECT_EQ(4, rows[1]);
  EXPECT_EQ(2, rows[2]);
  EXPECT_EQ("strawberry-queue-rows:1,4", QueueDrop::RowsPayload({1, 4}, QueueDrop::kQueueRowsPrefix));
  EXPECT_TRUE(QueueDrop::ParseUrls("strawberry-playlist-rows:0").empty());
  const std::vector<std::string> urls = QueueDrop::ParseUrls("file:///a\r\nfile:///b\n");
  ASSERT_EQ(2u, urls.size());
  EXPECT_EQ("file:///a", urls[0]);
  EXPECT_EQ("file:///b", urls[1]);
  EXPECT_EQ(1, QueueDrop::DestinationAfterRemove(3, {0, 2}));
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

TEST(PlaylistManager, SongChangeRequestProcessedGreysCurrent) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  manager.AppendSongs({MakeSong("A", "file:///a")});
  manager.SetCurrentRow(0);
  manager.SongChangeRequestProcessed("file:///a", false);
  EXPECT_TRUE(manager.current()->current_song().unavailable());
  manager.SongChangeRequestProcessed("file:///a", true);
  EXPECT_FALSE(manager.current()->current_song().unavailable());
}

TEST(QueueRows, RemapsAndPositionsPlaylistSources) {
  const std::vector<QueueRows::Source> sources = {{1, 0}, {1, 2}, {2, 1}, {-1, -1}};
  EXPECT_EQ(1, QueueRows::PositionForRow(sources, 1, 0));
  EXPECT_EQ(2, QueueRows::PositionForRow(sources, 1, 2));
  EXPECT_EQ(0, QueueRows::PositionForRow(sources, 1, 1));
  const auto removed = QueueRows::AfterRemove(sources, 1, {0});
  ASSERT_EQ(3u, removed.size());
  EXPECT_EQ(1, removed[0].row);
  EXPECT_EQ(2, removed[1].playlist_id);
  const auto inserted = QueueRows::AfterInsert({{1, 2}}, 1, 1, 2);
  ASSERT_EQ(1u, inserted.size());
  EXPECT_EQ(4, inserted.front().row);
  const auto moved = QueueRows::AfterMove({{1, 1}, {1, 3}}, 1, 4, {1}, 4);
  ASSERT_EQ(2u, moved.size());
  EXPECT_EQ(3, moved.front().row);
  EXPECT_EQ(2, moved.back().row);
}

TEST(Queue, TracksPlaylistRowsAndToggle) {
  Queue queue;
  Song a = MakeSong("A", "file:///a");
  Song b = MakeSong("B", "file:///b");
  queue.Append(a, 4, 1);
  queue.Append(b, 4, 3);
  EXPECT_EQ(1, queue.PositionForPlaylistRow(4, 1));
  EXPECT_EQ(2, queue.PositionForPlaylistRow(4, 3));
  EXPECT_TRUE(queue.ContainsPlaylistRow(4, 1));
  queue.TogglePlaylistRow(4, 1, a);
  EXPECT_FALSE(queue.ContainsPlaylistRow(4, 1));
  EXPECT_EQ(1, queue.PositionForPlaylistRow(4, 3));
  queue.RemapAfterPlaylistRemove(4, {3});
  EXPECT_TRUE(queue.empty());
}

TEST(PlaylistDelegates, QueueColumnTitle) {
  EXPECT_EQ("Queue", PlaylistDelegates::ColumnTitle(PlaylistColumn::Queue));
  EXPECT_TRUE(PlaylistDelegates::ColumnText(MakeSong("A", "file:///a"), PlaylistColumn::Queue).empty());
}

TEST(PlaylistBackend, PersistsUuidColumnsAndDynamicSearch) {
  const std::string path = "/tmp/strawberry-playlist-persist.db";
  FileUtils::Remove(path);
  Database db(path);
  ASSERT_TRUE(db.Open());
  TagReader tagreader;
  PlaylistBackend backend(&db, &tagreader, nullptr);
  Playlist playlist;
  playlist.set_name("Dynamic Roads");
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_year(1994);
  song.set_url("file:///roads.flac");
  song.set_lyrics("hidden");
  song.set_valid(true);
  playlist.AppendSongs({song});
  SmartPlaylistSearch search;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "Portishead"});
  playlist.SetDynamic(true, search);
  const int id = backend.SavePlaylist(&playlist);
  ASSERT_GT(id, 0);
  EXPECT_TRUE(PlaylistItemUuid::Valid(playlist.UuidAt(0)));
  auto loaded = backend.LoadPlaylist(id);
  ASSERT_NE(nullptr, loaded);
  ASSERT_EQ(1, loaded->row_count());
  EXPECT_EQ("Roads", loaded->song(0).title());
  EXPECT_EQ("Portishead", loaded->song(0).artist());
  EXPECT_EQ(1994, loaded->song(0).year());
  EXPECT_EQ("hidden", loaded->song(0).lyrics());
  EXPECT_TRUE(loaded->is_dynamic());
  ASSERT_FALSE(loaded->dynamic_search().terms.empty());
  EXPECT_EQ("Portishead", loaded->dynamic_search().terms.front().value);
  EXPECT_EQ(playlist.UuidAt(0), loaded->UuidAt(0));
  Song edited = loaded->song(0);
  edited.set_title("Glory Box");
  loaded->ReplaceRow(0, edited);
  backend.SavePlaylistItems(id, loaded->uuids(), loaded->songs());
  auto updated = backend.LoadPlaylist(id);
  EXPECT_EQ("Glory Box", updated->song(0).title());
  EXPECT_EQ(loaded->UuidAt(0), updated->UuidAt(0));
  FileUtils::Remove(path);
}

TEST(PlaylistDragPayload, EncodesSourceAndDetectsCrossPlaylist) {
  const std::string encoded = PlaylistDragPayload::Encode(7, {0, 2});
  EXPECT_EQ("strawberry-playlist-rows:7|0,2", encoded);
  const PlaylistDragPayload::Payload decoded = PlaylistDragPayload::Decode(encoded);
  EXPECT_EQ(7, decoded.source_id);
  ASSERT_EQ(2u, decoded.rows.size());
  EXPECT_EQ(0, decoded.rows[0]);
  EXPECT_EQ(2, decoded.rows[1]);
  const PlaylistDragPayload::Payload legacy = PlaylistDragPayload::Decode("strawberry-playlist-rows:0,3");
  EXPECT_EQ(-1, legacy.source_id);
  ASSERT_EQ(2u, legacy.rows.size());
  EXPECT_EQ(0, legacy.rows[0]);
  EXPECT_EQ(3, legacy.rows[1]);
  EXPECT_TRUE(PlaylistDragPayload::IsCrossPlaylist(7, 3));
  EXPECT_FALSE(PlaylistDragPayload::IsCrossPlaylist(7, 7));
  EXPECT_FALSE(PlaylistDragPayload::IsCrossPlaylist(-1, 3));
  EXPECT_EQ(std::vector<int>({0, 2}), PlaylistListDrop::ParsePlaylistRows(encoded));
}

TEST(PlaylistManager, MoveRowsBetweenRemovesFromSource) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  Playlist *source = manager.current();
  ASSERT_NE(nullptr, source);
  source->AppendSongs({MakeSong("A", "file:///a"), MakeSong("B", "file:///b"), MakeSong("C", "file:///c")});
  Playlist *dest = manager.New("Dest");
  ASSERT_NE(nullptr, dest);
  manager.MoveRowsBetween(source->id(), dest->id(), {0, 2});
  ASSERT_EQ(1, source->row_count());
  EXPECT_EQ("B", source->song(0).title());
  ASSERT_EQ(2, dest->row_count());
  EXPECT_EQ("A", dest->song(0).title());
  EXPECT_EQ("C", dest->song(1).title());
  EXPECT_TRUE(source->CanUndo());
  EXPECT_TRUE(dest->CanUndo());
  dest->Undo();
  source->Undo();
  EXPECT_EQ(3, source->row_count());
  EXPECT_EQ(0, dest->row_count());
}

TEST(PlaylistManager, UndoCrossMoveRestoresBoth) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  Playlist *source = manager.current();
  ASSERT_NE(nullptr, source);
  source->AppendSongs({MakeSong("A", "file:///a"), MakeSong("B", "file:///b"), MakeSong("C", "file:///c")});
  Playlist *dest = manager.New("Dest");
  ASSERT_NE(nullptr, dest);
  manager.MoveRowsBetween(source->id(), dest->id(), {0, 2});
  ASSERT_EQ(1, source->row_count());
  ASSERT_EQ(2, dest->row_count());
  EXPECT_TRUE(PlaylistCrossUndoPair::ShouldPairUndo(source->id(), dest->id()));
  EXPECT_FALSE(PlaylistCrossUndoPair::ShouldPairUndo(source->id(), source->id()));
  EXPECT_FALSE(PlaylistCrossUndoPair::ShouldBypass(2));
  EXPECT_TRUE(manager.UndoCrossMove(source->id(), dest->id()));
  EXPECT_EQ(3, source->row_count());
  EXPECT_EQ(0, dest->row_count());
  EXPECT_EQ("A", source->song(0).title());
  EXPECT_EQ("B", source->song(1).title());
  EXPECT_EQ("C", source->song(2).title());
}

TEST(PlaylistManager, UpdateCollectionSongsPatchesAllPlaylists) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  Playlist *first = manager.current();
  ASSERT_NE(nullptr, first);
  Song song;
  song.set_id(21);
  song.set_title("Roads");
  song.set_url("file:///roads.flac");
  song.set_valid(true);
  first->AppendSongs({song});
  Playlist *second = manager.New("Second");
  ASSERT_NE(nullptr, second);
  second->AppendSongs({song});
  Song updated = song;
  updated.set_artist("Portishead");
  manager.UpdateCollectionSongs({updated});
  EXPECT_EQ("Portishead", first->song(0).artist());
  EXPECT_EQ("Portishead", second->song(0).artist());
}

TEST(PlaylistManager, EachPlaylistKeepsItsOwnQueue) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  Playlist *first = manager.current();
  ASSERT_NE(nullptr, first);
  first->queue()->Append(MakeSong("A", "file:///a"));
  Playlist *second = manager.New("Second");
  ASSERT_NE(nullptr, second);
  second->queue()->Append(MakeSong("B", "file:///b"));
  EXPECT_EQ(1, first->queue()->size());
  EXPECT_EQ("A", first->queue()->songs().front().title());
  EXPECT_EQ(1, second->queue()->size());
  EXPECT_EQ("B", second->queue()->songs().front().title());
  EXPECT_EQ(second->queue(), PlaylistQueueScope::For(manager.current()));
  manager.SetCurrentPlaylist(first->id());
  EXPECT_EQ(first->queue(), PlaylistQueueScope::For(manager.current()));
}
