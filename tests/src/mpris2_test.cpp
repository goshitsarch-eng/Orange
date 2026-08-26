#include "mpris2/mpris2helpers.h"

#include <gtest/gtest.h>

TEST(Mpris2Helpers, TrackIdAndArtUrl) {
  Song song;
  song.set_id(12);
  song.set_art_automatic("file:///covers/auto.jpg");
  EXPECT_EQ("/org/strawberrymusicplayer/Strawberry/Track/12", Mpris2Helpers::TrackId(song));
  EXPECT_EQ("file:///covers/auto.jpg", Mpris2Helpers::ArtUrl(song));
  song.set_art_manual("file:///covers/manual.jpg");
  EXPECT_EQ("file:///covers/manual.jpg", Mpris2Helpers::ArtUrl(song));
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

TEST(Mpris2Helpers, LoopStatusRoundTrip) {
  EXPECT_EQ("None", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Off));
  EXPECT_EQ("Track", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Track));
  EXPECT_EQ("Playlist", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Playlist));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Track, Mpris2Helpers::RepeatFromLoopStatus("Track"));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Playlist, Mpris2Helpers::RepeatFromLoopStatus("Playlist"));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Off, Mpris2Helpers::RepeatFromLoopStatus("None"));
}
