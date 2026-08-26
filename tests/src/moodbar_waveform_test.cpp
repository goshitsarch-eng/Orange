#include "constants/moodbarsettings.h"
#include "constants/waveformsettings.h"
#include "core/song.h"
#include "moodbar/moodbarcell.h"
#include "moodbar/moodbarpaths.h"
#include "moodbar/moodbarstyle.h"
#include "utilities/analysisasync.h"
#include "waveform/waveformstyle.h"

#include <gtest/gtest.h>

TEST(MoodbarStyle, NamesAndClampMatchQt) {
  EXPECT_STREQ("Normal", MoodbarStyle::StyleName(MoodbarSettings::Style::Normal));
  EXPECT_STREQ("Angry", MoodbarStyle::StyleName(MoodbarSettings::Style::Angry));
  EXPECT_STREQ("Frozen", MoodbarStyle::StyleName(MoodbarSettings::Style::Frozen));
  EXPECT_STREQ("Happy", MoodbarStyle::StyleName(MoodbarSettings::Style::Happy));
  EXPECT_STREQ("System colors", MoodbarStyle::StyleName(MoodbarSettings::Style::SystemPalette));
  EXPECT_EQ(MoodbarSettings::Style::Normal, MoodbarStyle::ClampStyle(-1));
  EXPECT_EQ(MoodbarSettings::Style::Happy, MoodbarStyle::ClampStyle(3));
  EXPECT_EQ(MoodbarSettings::Style::Normal, MoodbarStyle::ClampStyle(99));
}

TEST(MoodbarStyle, PropertiesMatchQtCoefficients) {
  const auto angry = MoodbarStyle::PropertiesFor(MoodbarSettings::Style::Angry, 720);
  EXPECT_EQ(720 / 360 * 9, angry.threshold);
  EXPECT_EQ(45, angry.range_start);
  EXPECT_EQ(-45, angry.range_delta);
  EXPECT_EQ(200, angry.sat);
  const auto frozen = MoodbarStyle::PropertiesFor(MoodbarSettings::Style::Frozen, 720);
  EXPECT_EQ(720 / 360 * 1, frozen.threshold);
  EXPECT_EQ(140, frozen.range_start);
  EXPECT_EQ(50, frozen.sat);
  const auto happy = MoodbarStyle::PropertiesFor(MoodbarSettings::Style::Happy, 360);
  EXPECT_EQ(2, happy.threshold);
  EXPECT_EQ(359, happy.range_delta);
  EXPECT_EQ(250, happy.val);
}

TEST(MoodbarStyle, ApplyRemapsAngryAwayFromNormal) {
  const std::vector<uint8_t> mood = {255, 0, 0, 0, 255, 0, 0, 0, 255};
  const std::vector<uint8_t> normal = MoodbarStyle::Apply(mood, MoodbarSettings::Style::Normal);
  const std::vector<uint8_t> angry = MoodbarStyle::Apply(mood, MoodbarSettings::Style::Angry);
  ASSERT_EQ(mood.size(), normal.size());
  ASSERT_EQ(mood.size(), angry.size());
  EXPECT_NE(normal, angry);
  EXPECT_TRUE(MoodbarStyle::Apply({}, MoodbarSettings::Style::Normal).empty());
}

TEST(MoodbarPaths, SidecarsUseCompleteBaseName) {
  EXPECT_EQ("/music/.song.mood", MoodbarPaths::HiddenSidecar("/music/song.flac"));
  EXPECT_EQ("/music/song.mood", MoodbarPaths::VisibleSidecar("/music/song.flac"));
  const auto sidecars = MoodbarPaths::Sidecars("/music/song.flac");
  ASSERT_EQ(2u, sidecars.size());
  EXPECT_EQ("/music/.song.mood", sidecars.front());
  EXPECT_EQ("/tmp/cache/track.mp3.mood", MoodbarPaths::CacheFile("/tmp/cache", "file:///tmp/track.mp3"));
}

TEST(WaveformStyle, ColorCurveAndSidecars) {
  EXPECT_STREQ("#6696e3", WaveformSettings::kDefaultColor);
  const ColorUtils::Rgb color = WaveformStyle::BarColorFromHex(WaveformSettings::kDefaultColor);
  EXPECT_EQ(0x66, color.r);
  EXPECT_EQ(0x96, color.g);
  EXPECT_EQ(0xe3, color.b);
  EXPECT_EQ(WaveformStyle::DefaultBarColor().r, WaveformStyle::BarColorFromHex("").r);
  EXPECT_NEAR(1.0, WaveformStyle::ShapedAmplitude(1.0f), 0.001);
  EXPECT_GT(WaveformStyle::ShapedAmplitude(0.25f), 0.25);
  EXPECT_LT(WaveformStyle::ShapedAmplitude(0.25f), 0.6);
  EXPECT_EQ("/music/.song.flac.waveform", WaveformStyle::HiddenSidecar("/music/song.flac"));
  EXPECT_EQ("/music/song.flac.waveform", WaveformStyle::VisibleSidecar("/music/song.flac"));
  EXPECT_EQ("/tmp/cache/song.flac.wave", WaveformStyle::CacheFile("/tmp/cache", "file:///music/song.flac"));
}

TEST(MoodbarCell, LoadRulesMatchQtDelegate) {
  EXPECT_FALSE(MoodbarCell::IsLocalUrl(""));
  EXPECT_FALSE(MoodbarCell::IsLocalUrl("https://example.com/a.mp3"));
  EXPECT_TRUE(MoodbarCell::IsLocalUrl("file:///tmp/a.flac"));
  EXPECT_TRUE(MoodbarCell::IsLocalUrl("/tmp/a.flac"));
  EXPECT_EQ(16, MoodbarCell::ColumnHeight());
  EXPECT_EQ(120, MoodbarCell::ColumnWidth());
  EXPECT_EQ(1, MoodbarCell::BorderInset());
  EXPECT_STREQ("", MoodbarCell::PlaceholderText());

  Song local;
  local.set_source(Song::Source::LocalFile);
  local.set_url("file:///tmp/a.flac");
  EXPECT_TRUE(MoodbarCell::CanLoad(local));
  EXPECT_EQ("file:///tmp/a.flac", MoodbarCell::CacheKey(local));

  Song collection;
  collection.set_source(Song::Source::Collection);
  collection.set_url("file:///music/b.flac");
  EXPECT_TRUE(MoodbarCell::CanLoad(collection));

  Song cue;
  cue.set_source(Song::Source::LocalFile);
  cue.set_url("file:///tmp/album.flac");
  cue.set_cue_path("/tmp/album.cue");
  EXPECT_FALSE(MoodbarCell::CanLoad(cue));

  Song stream;
  stream.set_source(Song::Source::Stream);
  stream.set_url("http://example.com/radio");
  EXPECT_FALSE(MoodbarCell::CanLoad(stream));

  Song tidal;
  tidal.set_source(Song::Source::Tidal);
  tidal.set_url("tidal://track/1");
  EXPECT_FALSE(MoodbarCell::CanLoad(tidal));

  Song cdda;
  cdda.set_source(Song::Source::CDDA);
  cdda.set_url("cdda://1");
  EXPECT_FALSE(MoodbarCell::CanLoad(cdda));

  EXPECT_EQ(MoodbarCell::State::CannotLoad, MoodbarCell::NextState(MoodbarCell::State::None, false, false, false));
  EXPECT_EQ(MoodbarCell::State::Loaded, MoodbarCell::NextState(MoodbarCell::State::Loading, true, true, false));
  EXPECT_EQ(MoodbarCell::State::Loading, MoodbarCell::NextState(MoodbarCell::State::None, true, false, true));
  EXPECT_EQ(MoodbarCell::State::None, MoodbarCell::NextState(MoodbarCell::State::None, true, false, false));
}

TEST(AnalysisAsync, GenerateOnlyWhenEnabledAndCacheMisses) {
  EXPECT_TRUE(AnalysisAsync::NeedsGenerate(true, false));
  EXPECT_FALSE(AnalysisAsync::NeedsGenerate(true, true));
  EXPECT_FALSE(AnalysisAsync::NeedsGenerate(false, false));
  EXPECT_TRUE(AnalysisAsync::AcceptGeneration(3, 3, true));
  EXPECT_FALSE(AnalysisAsync::AcceptGeneration(2, 3, true));
  EXPECT_FALSE(AnalysisAsync::AcceptGeneration(3, 3, false));
}
