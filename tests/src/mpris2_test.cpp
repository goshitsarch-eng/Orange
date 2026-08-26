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
}

TEST(Mpris2Helpers, LoopStatusRoundTrip) {
  EXPECT_EQ("None", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Off));
  EXPECT_EQ("Track", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Track));
  EXPECT_EQ("Playlist", Mpris2Helpers::LoopStatus(PlaylistSequence::RepeatMode::Playlist));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Track, Mpris2Helpers::RepeatFromLoopStatus("Track"));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Playlist, Mpris2Helpers::RepeatFromLoopStatus("Playlist"));
  EXPECT_EQ(PlaylistSequence::RepeatMode::Off, Mpris2Helpers::RepeatFromLoopStatus("None"));
}
