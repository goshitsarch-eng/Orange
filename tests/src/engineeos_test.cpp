#include "engine/engineeos.h"

#include <gtest/gtest.h>

TEST(EngineEos, FadeoutPipelineIsJustReaped) {
  EXPECT_EQ(EngineEos::Action::RemoveFadeout, EngineEos::ActionFor(true, false, false));
  EXPECT_EQ(EngineEos::Action::RemoveFadeout, EngineEos::ActionFor(true, true, true));
  EXPECT_FALSE(EngineEos::EmitsTrackEnded(EngineEos::Action::RemoveFadeout));
}

TEST(EngineEos, EndOfStreamOnAnotherPipelineIsIgnored) {
  EXPECT_EQ(EngineEos::Action::Ignore, EngineEos::ActionFor(false, false, false));
  EXPECT_EQ(EngineEos::Action::Ignore, EngineEos::ActionFor(false, false, true));
  EXPECT_FALSE(EngineEos::EmitsTrackEnded(EngineEos::Action::Ignore));
}

// A preloaded second pipeline is built but never started, so end-of-stream on the current pipeline ends the
// track: promoting the silent pipeline instead left playback stopped on every track boundary.
TEST(EngineEos, PreloadedPipelineDoesNotSuppressTheTrackChange) {
  EXPECT_EQ(EngineEos::Action::EndTrack, EngineEos::ActionFor(false, true, false));
  EXPECT_TRUE(EngineEos::EmitsTrackEnded(EngineEos::Action::EndTrack));
}

// The player has to advance the playlist even when playbin continued into the next URL by itself.
TEST(EngineEos, GaplessContinuationStillEndsTheTrack) {
  EXPECT_EQ(EngineEos::Action::ContinueGapless, EngineEos::ActionFor(false, true, true));
  EXPECT_TRUE(EngineEos::EmitsTrackEnded(EngineEos::Action::ContinueGapless));
}

TEST(EngineEos, FinishedPipelineIsNeverGivenANextUri) {
  EXPECT_TRUE(EngineEos::CanContinueIntoNextUri(true, true, false, false));
  EXPECT_FALSE(EngineEos::CanContinueIntoNextUri(true, true, true, false));
  EXPECT_FALSE(EngineEos::CanContinueIntoNextUri(false, true, false, false));
  EXPECT_FALSE(EngineEos::CanContinueIntoNextUri(true, false, false, false));
  EXPECT_FALSE(EngineEos::CanContinueIntoNextUri(true, true, false, true));
}

TEST(EngineEos, PreloadedPipelineIsAdoptedOnlyForTheSameUrl) {
  EXPECT_TRUE(EngineEos::ShouldAdoptPreloaded(true, "file:///b.flac", "file:///b.flac"));
  EXPECT_FALSE(EngineEos::ShouldAdoptPreloaded(true, "file:///b.flac", "file:///c.flac"));
  EXPECT_FALSE(EngineEos::ShouldAdoptPreloaded(false, "file:///b.flac", "file:///b.flac"));
  EXPECT_FALSE(EngineEos::ShouldAdoptPreloaded(true, "", ""));
}

TEST(EngineEos, AlreadyPlayingOnlyShortCircuitsAContinuedTrackChange) {
  EXPECT_TRUE(EngineEos::AlreadyPlaying(true, true, true, "file:///b.flac", "file:///b.flac"));
  // A manual re-selection of the same song has to restart it, not be swallowed.
  EXPECT_FALSE(EngineEos::AlreadyPlaying(true, false, true, "file:///b.flac", "file:///b.flac"));
  EXPECT_FALSE(EngineEos::AlreadyPlaying(false, true, true, "file:///b.flac", "file:///b.flac"));
  EXPECT_FALSE(EngineEos::AlreadyPlaying(true, true, true, "file:///b.flac", "file:///c.flac"));
  EXPECT_FALSE(EngineEos::AlreadyPlaying(true, true, false, "file:///b.flac", "file:///b.flac"));
}
