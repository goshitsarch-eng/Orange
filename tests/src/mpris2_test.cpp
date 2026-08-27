#include "mpris2/mpris2helpers.h"
#include "mpris2/mpris2playlists.h"
#include "playlist/playlist.h"

#include <gtest/gtest.h>

TEST(Mpris2Helpers, TrackIdAndArtUrl) {
  Song song;
  song.set_id(12);
  song.set_art_automatic("file:///covers/auto.jpg");
  EXPECT_EQ("/org/strawberrymusicplayer/Strawberry/Track/12", Mpris2Helpers::TrackId(song));
  EXPECT_EQ("file:///covers/auto.jpg", Mpris2Helpers::ArtUrl(song));
  song.set_art_manual("file:///covers/manual.jpg");
  EXPECT_EQ("file:///covers/manual.jpg", Mpris2Helpers::ArtUrl(song));
  Song embedded;
  EXPECT_EQ("file:///tmp/current-albumcover.jpg", Mpris2Helpers::ArtUrlOrOverride(embedded, "file:///tmp/current-albumcover.jpg"));
  EXPECT_EQ("file:///covers/manual.jpg", Mpris2Helpers::ArtUrlOrOverride(song, "file:///tmp/current-albumcover.jpg"));
  Song local;
  EXPECT_EQ("/org/strawberrymusicplayer/Strawberry/Track/row3", Mpris2Helpers::TrackIdForRow(local, 3));
  EXPECT_EQ(3, Mpris2Helpers::RowFromTrackId("/org/strawberrymusicplayer/Strawberry/Track/row3"));
  EXPECT_EQ(-1, Mpris2Helpers::RowFromTrackId(Mpris2Helpers::TrackId(song)));
}

TEST(Mpris2Helpers, PositionUsec) {
  EXPECT_EQ(0, Mpris2Helpers::PositionUsec(0));
  EXPECT_EQ(1500000, Mpris2Helpers::PositionUsec(1500000000LL));
}

TEST(Mpris2Helpers, DiffTrackIdsIncrementalAndReorder) {
  EXPECT_EQ(Mpris2Helpers::TrackListDiff::Kind::None, Mpris2Helpers::DiffTrackIds({"a"}, {"a"}).kind);
  const auto added = Mpris2Helpers::DiffTrackIds({"a"}, {"a", "b"});
  EXPECT_EQ(Mpris2Helpers::TrackListDiff::Kind::Incremental, added.kind);
  ASSERT_EQ(1u, added.added.size());
  EXPECT_EQ("b", added.added[0]);
  EXPECT_EQ("a", added.after_track[0]);
  const auto front = Mpris2Helpers::DiffTrackIds({"a"}, {"b", "a"});
  EXPECT_EQ(Mpris2Helpers::TrackListDiff::Kind::Incremental, front.kind);
  EXPECT_EQ("b", front.added[0]);
  EXPECT_EQ(Mpris2Helpers::kNoTrack, front.after_track[0]);
  const auto removed = Mpris2Helpers::DiffTrackIds({"a", "b"}, {"a"});
  EXPECT_EQ(Mpris2Helpers::TrackListDiff::Kind::Incremental, removed.kind);
  ASSERT_EQ(1u, removed.removed.size());
  EXPECT_EQ("b", removed.removed[0]);
  const auto reordered = Mpris2Helpers::DiffTrackIds({"a", "b"}, {"b", "a"});
  EXPECT_EQ(Mpris2Helpers::TrackListDiff::Kind::Replaced, reordered.kind);
}

TEST(Mpris2Helpers, MetadataNeedsUpdate) {
  Song a;
  a.set_title("Roads");
  a.set_artist("Portishead");
  Song b = a;
  EXPECT_FALSE(Mpris2Helpers::MetadataNeedsUpdate(a, b));
  b.set_title("Glory Box");
  EXPECT_TRUE(Mpris2Helpers::MetadataNeedsUpdate(a, b));
}

TEST(Mpris2Helpers, CapabilitiesMatchQt) {
  EXPECT_FALSE(Mpris2Helpers::CanPlay(nullptr));
  Playlist playlist;
  EXPECT_FALSE(Mpris2Helpers::CanPlay(&playlist));
  Song first;
  first.set_title("A");
  first.set_url("file:///a.flac");
  first.set_valid(true);
  Song second = first;
  second.set_title("B");
  second.set_url("file:///b.flac");
  playlist.AppendSongs({first, second});
  playlist.set_current_row(0);
  EXPECT_TRUE(Mpris2Helpers::CanPlay(&playlist));
  EXPECT_TRUE(Mpris2Helpers::CanGoNext(&playlist));
  EXPECT_FALSE(Mpris2Helpers::CanGoPrevious(&playlist, 0));
  EXPECT_TRUE(Mpris2Helpers::CanGoPrevious(&playlist, 4LL * 1000000000LL));
  EXPECT_TRUE(Mpris2Helpers::PreviousWouldRestartTrack(4LL * 1000000000LL));
  playlist.set_current_row(1);
  EXPECT_EQ(-1, playlist.PeekNextRow());
  EXPECT_FALSE(Mpris2Helpers::CanGoNext(&playlist));
  EXPECT_TRUE(Mpris2Helpers::CanPause(EngineBase::State::Playing));
  EXPECT_TRUE(Mpris2Helpers::CanPause(EngineBase::State::Idle));
  EXPECT_FALSE(Mpris2Helpers::CanPause(EngineBase::State::Empty));
  EXPECT_TRUE(Mpris2Helpers::CanSeek(first, EngineBase::State::Playing));
  EXPECT_FALSE(Mpris2Helpers::CanSeek(first, EngineBase::State::Empty));
  Song stream;
  stream.set_valid(true);
  stream.set_source(Song::Source::Stream);
  EXPECT_FALSE(Mpris2Helpers::CanSeek(stream, EngineBase::State::Playing));
  EXPECT_TRUE(Mpris2Helpers::SetPositionAllowed("/id", "/id", 1000, 5000000000LL, true));
  EXPECT_FALSE(Mpris2Helpers::SetPositionAllowed("/other", "/id", 1000, 5000000000LL, true));
  EXPECT_FALSE(Mpris2Helpers::SetPositionAllowed("/id", "/id", -1, 5000000000LL, true));
  EXPECT_FALSE(Mpris2Helpers::SetPositionAllowed("/id", "/id", 1000, 5000000000LL, false));
  EXPECT_FALSE(Mpris2Helpers::SetPositionAllowed("/id", "/id", 6000000, 5000000000LL, true));
}

TEST(Mpris2Playlists, PathPageAndOrder) {
  EXPECT_EQ("/org/strawberrymusicplayer/strawberry/PlaylistId/7", Mpris2Playlists::ObjectPath(7));
  EXPECT_EQ(7, Mpris2Playlists::IdFromPath("/org/strawberrymusicplayer/strawberry/PlaylistId/7"));
  EXPECT_EQ(-1, Mpris2Playlists::IdFromPath("/org/mpris/MediaPlayer2"));
  EXPECT_TRUE(Mpris2Playlists::IsAlphabetical("Alphabetical"));
  EXPECT_FALSE(Mpris2Playlists::IsAlphabetical("UserDefined"));
  std::vector<Mpris2Playlists::Entry> entries = {
      {Mpris2Playlists::ObjectPath(2), "Zed", {}},
      {Mpris2Playlists::ObjectPath(1), "Alpha", {}},
  };
  const auto alpha = Mpris2Playlists::SortAndSlice(entries, "Alphabetical", false, 0, 10);
  ASSERT_EQ(2u, alpha.size());
  EXPECT_EQ("Alpha", alpha[0].name);
  EXPECT_EQ("Zed", alpha[1].name);
  const auto user = Mpris2Playlists::SortAndSlice(entries, "UserDefined", false, 0, 10);
  EXPECT_EQ(Mpris2Playlists::ObjectPath(1), user[0].id);
  const auto page = Mpris2Playlists::SortAndSlice(entries, "Alphabetical", false, 1, 1);
  ASSERT_EQ(1u, page.size());
  EXPECT_EQ("Zed", page[0].name);
  const auto reversed = Mpris2Playlists::SortAndSlice(entries, "Alphabetical", true, 0, 1);
  ASSERT_EQ(1u, reversed.size());
  EXPECT_EQ("Zed", reversed[0].name);
  EXPECT_TRUE(Mpris2Playlists::SortAndSlice(entries, "Alphabetical", false, 5, 2).empty());
  EXPECT_EQ(2u, Mpris2Playlists::Orderings().size());
}

TEST(Mpris2Helpers, LoopStatusRoundTrip) {
  EXPECT_EQ("None", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Off));
  EXPECT_EQ("Track", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Track));
  EXPECT_EQ("Playlist", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Playlist));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Track, Mpris2Helpers::RepeatFromLoopStatus("Track"));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Playlist, Mpris2Helpers::RepeatFromLoopStatus("Playlist"));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Off, Mpris2Helpers::RepeatFromLoopStatus("None"));
}
