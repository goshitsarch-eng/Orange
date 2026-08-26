#include "core/playerrepeat.h"
#include "playlist/playlist.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistshuffle.h"
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
  playlist.set_current_row(0);
  Song more;
  more.set_title("More");
  more.set_artist("A Group");
  more.set_url("file:///more");
  more.set_valid(true);
  playlist.ExpandDynamic({have, extra, skip, more});
  EXPECT_EQ(3, playlist.row_count());
  playlist.RepopulateDynamic({have, extra, skip, more});
  EXPECT_GE(playlist.row_count(), 1);
  playlist.SetDynamic(false);
  EXPECT_FALSE(playlist.is_dynamic());
  EXPECT_EQ(Playlist::SequenceMode::Sequential, playlist.sequence_mode());
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

TEST(Playlist, MoveRowsKeepsPlayingTrack) {
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
  playlist.MoveRows({0}, 2);
  ASSERT_EQ(3, playlist.row_count());
  EXPECT_EQ("B", playlist.songs()[0].title());
  EXPECT_EQ("A", playlist.songs()[1].title());
  EXPECT_EQ("C", playlist.songs()[2].title());
  EXPECT_EQ(1, playlist.current_row());
  playlist.MoveRows({2, 1}, 0);
  EXPECT_EQ("A", playlist.songs()[0].title());
  EXPECT_EQ("C", playlist.songs()[1].title());
  EXPECT_EQ("B", playlist.songs()[2].title());
  EXPECT_EQ(0, playlist.current_row());
  playlist.MoveRows({99}, 0);
  EXPECT_EQ("A", playlist.songs()[0].title());
}

TEST(Playlist, InvalidateDeletedSongsGreysMissingLocalFiles) {
  Playlist playlist;
  const std::string existing = "/tmp/strawberry-playlist-greyout-exists.txt";
  FileUtils::WriteFile(existing, "ok");
  Song keep;
  keep.set_title("Keep");
  keep.set_url(FileUtils::UriFromPath(existing));
  keep.set_valid(true);
  Song gone;
  gone.set_title("Gone");
  gone.set_url("file:///tmp/strawberry-playlist-greyout-missing.mp3");
  gone.set_valid(true);
  Song stream;
  stream.set_title("Stream");
  stream.set_url("http://example.invalid/live");
  stream.set_source(Song::Source::Stream);
  stream.set_valid(true);
  playlist.AppendSongs({keep, gone, stream});
  playlist.InvalidateDeletedSongs();
  ASSERT_EQ(3, playlist.row_count());
  EXPECT_FALSE(playlist.songs()[0].unavailable());
  EXPECT_TRUE(playlist.songs()[1].unavailable());
  EXPECT_FALSE(playlist.songs()[2].unavailable());
  FileUtils::Remove(existing);
}

TEST(Playlist, ApplyValidityOnCurrentSong) {
  Playlist playlist;
  Song song;
  song.set_title("A");
  song.set_url("file:///a");
  song.set_valid(true);
  playlist.AppendSongs({song});
  playlist.set_current_row(0);
  EXPECT_TRUE(playlist.ApplyValidityOnCurrentSong("file:///a", false));
  EXPECT_TRUE(playlist.current_song().unavailable());
  EXPECT_TRUE(playlist.ApplyValidityOnCurrentSong("file:///a", true));
  EXPECT_FALSE(playlist.current_song().unavailable());
}

TEST(Playlist, AutoSortAfterInsert) {
  Playlist playlist;
  playlist.set_auto_sort(true);
  playlist.SetSort(PlaylistColumn::Title, false);
  Song b;
  b.set_title("B");
  b.set_url("file:///b");
  b.set_valid(true);
  Song a;
  a.set_title("A");
  a.set_url("file:///a");
  a.set_valid(true);
  playlist.AppendSongs({b});
  playlist.AppendSongs({a});
  ASSERT_EQ(2, playlist.row_count());
  EXPECT_EQ("A", playlist.songs()[0].title());
  EXPECT_EQ("B", playlist.songs()[1].title());
}

TEST(Playlist, SetColumnValuesUpdatesSongsAndUndo) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_artist("One");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_artist("Two");
  b.set_url("file:///b");
  b.set_valid(true);
  playlist.AppendSongs({a, b});
  EXPECT_EQ(2, playlist.SetColumnValues({0, 1}, PlaylistColumn::Artist, "Shared"));
  EXPECT_EQ("Shared", playlist.songs()[0].artist());
  EXPECT_EQ("Shared", playlist.songs()[1].artist());
  EXPECT_TRUE(playlist.SetColumnValue(0, PlaylistColumn::Title, "Renamed"));
  EXPECT_EQ("Renamed", playlist.songs()[0].title());
  EXPECT_FALSE(playlist.SetColumnValue(0, PlaylistColumn::Bitrate, "128"));
  playlist.Undo();
  EXPECT_EQ("A", playlist.songs()[0].title());
  EXPECT_EQ("Shared", playlist.songs()[0].artist());
  playlist.Undo();
  EXPECT_EQ("One", playlist.songs()[0].artist());
  EXPECT_EQ("Two", playlist.songs()[1].artist());
}

TEST(PlaylistShuffle, AlbumAndGroupingKeys) {
  Song song;
  song.set_albumartist("Portishead");
  song.set_album("Dummy");
  EXPECT_EQ("Portishead\nDummy", PlaylistShuffle::AlbumKey(song));
  EXPECT_EQ("Portishead\nDummy", PlaylistShuffle::GroupingKey(song));
  song.set_grouping("Trip hop");
  EXPECT_EQ("Trip hop", PlaylistShuffle::GroupingKey(song));
}

TEST(PlaylistShuffle, AllUsesVirtualOrder) {
  Playlist playlist;
  for (const char *title : {"A", "B", "C", "D", "E"}) {
    Song song;
    song.set_title(title);
    song.set_url(std::string("file:///") + title);
    song.set_valid(true);
    playlist.AppendSongs({song});
  }
  playlist.set_current_row(0);
  playlist.SetShuffleMode(PlaylistSequence::ShuffleMode::All);
  playlist.Reshuffle(7);
  ASSERT_EQ(5u, playlist.virtual_items().size());
  EXPECT_EQ(0, playlist.virtual_items().front());
  EXPECT_NE((std::vector<int>{0, 1, 2, 3, 4}), playlist.virtual_items());
  playlist.Next();
  EXPECT_EQ(playlist.virtual_items()[1], playlist.current_row());
}

TEST(PlaylistShuffle, AlbumsKeepsTracksTogether) {
  Playlist playlist;
  auto add = [&](const char *title, const char *album) {
    Song song;
    song.set_title(title);
    song.set_album(album);
    song.set_albumartist("Artist");
    song.set_url(std::string("file:///") + title);
    song.set_valid(true);
    playlist.AppendSongs({song});
  };
  add("A1", "Alpha");
  add("A2", "Alpha");
  add("B1", "Beta");
  add("B2", "Beta");
  playlist.set_current_row(0);
  playlist.SetShuffleMode(PlaylistSequence::ShuffleMode::Albums);
  playlist.Reshuffle(3);
  const auto &order = playlist.virtual_items();
  ASSERT_EQ(4u, order.size());
  const std::string first = PlaylistShuffle::AlbumKey(playlist.song(order[0]));
  EXPECT_EQ(first, PlaylistShuffle::AlbumKey(playlist.song(order[1])));
  EXPECT_NE(first, PlaylistShuffle::AlbumKey(playlist.song(order[2])));
  EXPECT_EQ(PlaylistShuffle::AlbumKey(playlist.song(order[2])), PlaylistShuffle::AlbumKey(playlist.song(order[3])));
}

TEST(Playlist, FilterIsHonoredByNextAndPrevious) {
  Playlist playlist;
  auto add = [&](const char *title, const char *artist) {
    Song song;
    song.set_title(title);
    song.set_artist(artist);
    song.set_url(std::string("file:///") + title);
    song.set_valid(true);
    playlist.AppendSongs({song});
  };
  add("Keep", "Portishead");
  add("Skip", "Fleet Foxes");
  add("Also", "Portishead");
  playlist.set_current_row(0);
  playlist.SetFilterString("artist:Portishead");
  EXPECT_EQ(2, playlist.PeekNextRow());
  playlist.Next();
  EXPECT_EQ("Also", playlist.current_song().title());
  EXPECT_EQ(-1, playlist.PeekNextRow());
  playlist.SetRepeatMode(PlaylistSequence::RepeatMode::Playlist);
  EXPECT_EQ(0, playlist.PeekNextRow());
  playlist.SetRepeatMode(PlaylistSequence::RepeatMode::Off);
  playlist.set_current_row(2);
  EXPECT_EQ(-1, playlist.PeekNextRow());
  playlist.Previous();
  EXPECT_EQ("Keep", playlist.current_song().title());
}

TEST(Playlist, UpdateSongsByUrlReplacesMatchingRows) {
  Playlist playlist;
  Song a;
  a.set_title("Old");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("Other");
  b.set_url("file:///b");
  b.set_valid(true);
  playlist.AppendSongs({a, b});
  Song updated;
  updated.set_title("New");
  updated.set_artist("Portishead");
  updated.set_url("file:///a");
  updated.set_musicbrainz_recording_id("mbid-a");
  updated.set_valid(true);
  playlist.UpdateSongsByUrl(updated);
  EXPECT_EQ("New", playlist.song(0).title());
  EXPECT_EQ("mbid-a", playlist.song(0).musicbrainz_recording_id());
  EXPECT_EQ("Other", playlist.song(1).title());
}

TEST(PlayerRepeat, OneByOneStopsAfterCurrent) {
  EXPECT_TRUE(PlayerRepeat::ShouldStopAfterTrack(PlaylistSequence::RepeatMode::OneByOne, false));
  EXPECT_TRUE(PlayerRepeat::ShouldStopAfterTrack(PlaylistSequence::RepeatMode::Off, true));
  EXPECT_FALSE(PlayerRepeat::ShouldStopAfterTrack(PlaylistSequence::RepeatMode::Off, false));
  EXPECT_FALSE(PlayerRepeat::ShouldStopAfterTrack(PlaylistSequence::RepeatMode::Playlist, false));
}
