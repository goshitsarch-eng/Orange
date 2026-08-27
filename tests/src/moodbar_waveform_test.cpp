#include "constants/moodbarsettings.h"
#include "constants/waveformsettings.h"
#include "settings/moodbarsettingslabels.h"
#include "settings/waveformsettingslabels.h"
#include "widgets/seekbarmode.h"
#include "widgets/tracksliderwheel.h"
#include "core/song.h"
#include "moodbar/moodbarcell.h"
#include "moodbar/moodbarpaths.h"
#include "moodbar/moodbarplayhead.h"
#include "moodbar/moodbarpreview.h"
#include "moodbar/moodbarstyle.h"
#include "waveform/waveformplayhead.h"
#include "utilities/analysisasync.h"
#include "utilities/seekbaranalysis.h"
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

TEST(SeekbarAnalysis, CanLoadAndAcceptResult) {
  Song local;
  local.set_source(Song::Source::LocalFile);
  local.set_url("file:///tmp/a.flac");
  EXPECT_TRUE(SeekbarAnalysis::CanLoad(local));
  EXPECT_TRUE(SeekbarAnalysis::ShouldGenerate(true, local));
  EXPECT_FALSE(SeekbarAnalysis::ShouldGenerate(false, local));
  EXPECT_FALSE(SeekbarAnalysis::ShouldGenerate(true, Song()));
  EXPECT_TRUE(SeekbarAnalysis::ShouldGenerateOnEnable(true, false, local.url()));
  EXPECT_FALSE(SeekbarAnalysis::ShouldGenerateOnEnable(true, true, local.url()));
  EXPECT_FALSE(SeekbarAnalysis::ShouldGenerateOnEnable(true, false, {}));
  EXPECT_TRUE(SeekbarAnalysis::ShouldClearOnDisable(false, true));
  EXPECT_FALSE(SeekbarAnalysis::ShouldClearOnDisable(true, false));
  EXPECT_TRUE(SeekbarAnalysis::ShouldClearOnStop(true));
  EXPECT_FALSE(SeekbarAnalysis::ShouldClearOnStop(false));
  EXPECT_TRUE(SeekbarAnalysis::AcceptResult(true, true, local.url(), local.url(), 3, 3, true));
  EXPECT_FALSE(SeekbarAnalysis::AcceptResult(false, true, local.url(), local.url(), 3, 3, true));
  EXPECT_FALSE(SeekbarAnalysis::AcceptResult(true, false, local.url(), local.url(), 3, 3, true));
  EXPECT_FALSE(SeekbarAnalysis::AcceptResult(true, true, "file:///tmp/old.flac", local.url(), 3, 3, true));
  EXPECT_FALSE(SeekbarAnalysis::AcceptResult(true, true, local.url(), local.url(), 2, 3, true));

  Song cue;
  cue.set_source(Song::Source::LocalFile);
  cue.set_url("file:///tmp/album.flac");
  cue.set_cue_path("/tmp/album.cue");
  EXPECT_FALSE(SeekbarAnalysis::CanLoad(cue));
  EXPECT_TRUE(SeekbarAnalysis::ShouldGenerate(true, cue));

  Song stream;
  stream.set_source(Song::Source::Stream);
  stream.set_url("http://example.com/radio");
  EXPECT_FALSE(SeekbarAnalysis::CanLoad(stream));
}

TEST(AnalysisAsync, GenerateOnlyWhenEnabledAndCacheMisses) {
  EXPECT_TRUE(AnalysisAsync::NeedsGenerate(true, false));
  EXPECT_FALSE(AnalysisAsync::NeedsGenerate(true, true));
  EXPECT_FALSE(AnalysisAsync::NeedsGenerate(false, false));
  EXPECT_TRUE(AnalysisAsync::AcceptGeneration(3, 3, true));
  EXPECT_FALSE(AnalysisAsync::AcceptGeneration(2, 3, true));
  EXPECT_FALSE(AnalysisAsync::AcceptGeneration(3, 3, false));
}

TEST(TrackSliderWheel, AccumulatesQtNotches) {
  EXPECT_EQ(120, TrackSliderWheel::kRotationToSeek);
  const TrackSliderWheel::Result none = TrackSliderWheel::FromAngleDelta(0, 0);
  EXPECT_EQ(0, none.steps);
  const TrackSliderWheel::Result forward = TrackSliderWheel::FromAngleDelta(0, 120);
  EXPECT_EQ(1, forward.steps);
  EXPECT_EQ(TrackSliderWheel::Direction::Forward, TrackSliderWheel::DirectionFromSteps(forward.steps));
  const TrackSliderWheel::Result backward = TrackSliderWheel::FromAngleDelta(0, -120);
  EXPECT_EQ(-1, backward.steps);
  EXPECT_EQ(TrackSliderWheel::Direction::Backward, TrackSliderWheel::DirectionFromSteps(backward.steps));
  const TrackSliderWheel::Result partial = TrackSliderWheel::FromAngleDelta(0, 60);
  EXPECT_EQ(0, partial.steps);
  EXPECT_EQ(60, partial.accumulator);
  const TrackSliderWheel::Result completed = TrackSliderWheel::FromAngleDelta(partial.accumulator, 60);
  EXPECT_EQ(1, completed.steps);
  const TrackSliderWheel::Result gtk_up = TrackSliderWheel::FromGtkScroll(0, -1.0);
  EXPECT_EQ(1, gtk_up.steps);
  const TrackSliderWheel::Result gtk_down = TrackSliderWheel::FromGtkScroll(0, 1.0);
  EXPECT_EQ(-1, gtk_down.steps);
}

TEST(TrackSliderHover, MapsXToTimeAndDelta) {
  EXPECT_EQ(0, TrackSliderHover::SecondsAtX(0, 100, 180));
  EXPECT_EQ(90, TrackSliderHover::SecondsAtX(50, 100, 180));
  EXPECT_EQ(180, TrackSliderHover::SecondsAtX(100, 100, 180));
  EXPECT_EQ(0, TrackSliderHover::SecondsAtX(10, 0, 180));
  EXPECT_EQ("1:30", TrackSliderHover::HoverText(90));
  EXPECT_EQ("+0:30", TrackSliderHover::DeltaText(90, 60));
  EXPECT_EQ("-0:30", TrackSliderHover::DeltaText(30, 60));
}

TEST(SeekbarModeMenu, LabelsAndCycleMatchQt) {
  EXPECT_STREQ("Normal", SeekbarModeMenu::Label(SeekbarSettings::Mode::Normal));
  EXPECT_STREQ("Moodbar", SeekbarModeMenu::Label(SeekbarSettings::Mode::Moodbar));
  EXPECT_STREQ("Waveform", SeekbarModeMenu::Label(SeekbarSettings::Mode::Waveform));
  EXPECT_EQ(SeekbarSettings::Mode::Moodbar, SeekbarModeMenu::Next(SeekbarSettings::Mode::Normal));
  EXPECT_EQ(SeekbarSettings::Mode::Waveform, SeekbarModeMenu::Next(SeekbarSettings::Mode::Moodbar));
  EXPECT_EQ(SeekbarSettings::Mode::Normal, SeekbarModeMenu::Next(SeekbarSettings::Mode::Waveform));
  EXPECT_EQ(SeekbarSettings::Mode::Normal, SeekbarModeMenu::Clamp(-1));
  EXPECT_EQ(SeekbarSettings::Mode::Waveform, SeekbarModeMenu::Clamp(2));
  EXPECT_TRUE(SeekbarModeMenu::IsChecked(SeekbarSettings::Mode::Moodbar, SeekbarSettings::Mode::Moodbar));
  EXPECT_FALSE(SeekbarModeMenu::IsChecked(SeekbarSettings::Mode::Normal, SeekbarSettings::Mode::Waveform));
  EXPECT_TRUE(SeekbarModeMenu::StyleMenuEnabled(SeekbarSettings::Mode::Moodbar));
  EXPECT_FALSE(SeekbarModeMenu::StyleMenuEnabled(SeekbarSettings::Mode::Normal));
  EXPECT_STREQ("Moodbar style", SeekbarModeMenu::StyleSubmenuTitle());
}

TEST(WaveformPlayhead, SplitAndPlayedAlphaMatchQt) {
  EXPECT_EQ(1, WaveformPlayhead::kCursorWidth);
  EXPECT_FLOAT_EQ(0.55F, WaveformPlayhead::kPlayedAlpha);
  EXPECT_EQ(1000, WaveformPlayhead::kFadeDurationMs);
  EXPECT_DOUBLE_EQ(0.0, WaveformPlayhead::Progress(0, 10));
  EXPECT_DOUBLE_EQ(0.0, WaveformPlayhead::Progress(5, 0));
  EXPECT_DOUBLE_EQ(0.5, WaveformPlayhead::Progress(5, 10));
  EXPECT_DOUBLE_EQ(1.0, WaveformPlayhead::Progress(10, 10));
  EXPECT_DOUBLE_EQ(1.0, WaveformPlayhead::Progress(12, 10));
  EXPECT_EQ(0, WaveformPlayhead::SplitX(0, 100, 200));
  EXPECT_EQ(100, WaveformPlayhead::SplitX(50, 100, 200));
  EXPECT_EQ(200, WaveformPlayhead::SplitX(100, 100, 200));
  EXPECT_EQ(200, WaveformPlayhead::SplitX(150, 100, 200));
  EXPECT_EQ(0, WaveformPlayhead::SplitX(50, 0, 200));
  EXPECT_EQ(0, WaveformPlayhead::SplitX(50, 100, 0));
  EXPECT_FALSE(WaveformPlayhead::ShowPlayheadFromPosition(-1));
  EXPECT_TRUE(WaveformPlayhead::ShowPlayheadFromPosition(0));
}

TEST(MoodbarPlayhead, ArrowAndChromeMatchQt) {
  EXPECT_EQ(3, MoodbarPlayhead::kMarginSize);
  EXPECT_EQ(1, MoodbarPlayhead::kBorderSize);
  EXPECT_EQ(17, MoodbarPlayhead::kArrowWidth);
  EXPECT_EQ(13, MoodbarPlayhead::kArrowHeight);
  EXPECT_EQ(192, MoodbarPlayhead::InnerWidth(200));
  EXPECT_EQ(10, MoodbarPlayhead::InnerHeight(18));
  EXPECT_EQ(183, MoodbarPlayhead::ArrowTravel(200));
  EXPECT_EQ(0, MoodbarPlayhead::ArrowLeft(0, 100, 200));
  EXPECT_EQ(91, MoodbarPlayhead::ArrowLeft(50, 100, 200));
  EXPECT_EQ(183, MoodbarPlayhead::ArrowLeft(100, 100, 200));
  EXPECT_EQ(183, MoodbarPlayhead::ArrowLeft(150, 100, 200));
  EXPECT_EQ(0, MoodbarPlayhead::ArrowLeft(50, 0, 200));
  EXPECT_EQ(91 + 8, MoodbarPlayhead::ArrowCenterX(MoodbarPlayhead::ArrowLeft(50, 100, 200)));
  EXPECT_FALSE(MoodbarPlayhead::ShowPlayheadFromPosition(-1));
  EXPECT_TRUE(MoodbarPlayhead::ShowPlayheadFromPosition(0));
}

TEST(MoodbarPreview, SampleThumbnailsMatchQtSizeAndStyles) {
  EXPECT_EQ(150, MoodbarPreview::kWidth);
  EXPECT_EQ(18, MoodbarPreview::kHeight);
  EXPECT_STREQ("/org/strawberrymusicplayer/Strawberry/mood/sample.mood", MoodbarPreview::SampleResource());
  const std::vector<uint8_t> sample = MoodbarPreview::LoadSample();
  ASSERT_FALSE(sample.empty());
  EXPECT_EQ(0u, sample.size() % 3);
  EXPECT_EQ(static_cast<size_t>(MoodbarPreview::kWidth) * 3,
            MoodbarPreview::Stripe(sample, MoodbarSettings::Style::Normal, MoodbarPreview::kWidth).size());
  EXPECT_TRUE(MoodbarPreview::DistinctStyles(sample));
  EXPECT_TRUE(MoodbarPreview::Stripe({}, MoodbarSettings::Style::Normal, MoodbarPreview::kWidth).size() ==
              static_cast<size_t>(MoodbarPreview::kWidth) * 3);
}

TEST(MoodbarSettingsLabels, MatchQtStyleMenuAndSaveCopy) {
  EXPECT_STREQ("Moodbar style", MoodbarSettingsLabels::StyleLabel());
  EXPECT_STREQ("Save the .mood files directly in the songs folders", MoodbarSettingsLabels::SaveLabel());
  const auto choices = MoodbarSettingsLabels::StyleChoices();
  ASSERT_EQ(static_cast<size_t>(MoodbarSettings::Style::StyleCount), choices.size());
  EXPECT_EQ("0", choices.front().first);
  EXPECT_STREQ("Normal", choices.front().second.c_str());
  EXPECT_STREQ("System colors", choices.back().second.c_str());
}

TEST(WaveformSettingsLabels, MatchQtColorAndSaveCopy) {
  EXPECT_STREQ("Color", WaveformSettingsLabels::ColorTitle());
  EXPECT_STREQ("Select waveform color", WaveformSettingsLabels::ColorTooltip());
  EXPECT_STREQ("Save the .waveform files directly in the songs folders", WaveformSettingsLabels::SaveLabel());
}
