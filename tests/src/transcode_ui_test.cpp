#include "constants/transcodersettings.h"
#include "core/song.h"
#include "transcoder/transcodeui.h"
#include "transcoder/transcodelog.h"
#include "transcoder/transcoder.h"

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

TEST(TranscodeUi, TrimBasenameAndOutputPath) {
  EXPECT_EQ("roads", TranscodeUi::TrimBasename("/music/Dummy/roads.flac"));
  EXPECT_EQ("roads", TranscodeUi::TrimBasename("roads"));
  EXPECT_EQ("/out/roads.mp3", TranscodeUi::OutputPath("/music/Dummy/roads.flac", "/out", false, "/music", "mp3"));
  EXPECT_EQ("/out/Dummy/roads.mp3", TranscodeUi::OutputPath("/music/Dummy/roads.flac", "/out", true, "/music", "mp3"));
  EXPECT_EQ("/music/Dummy/roads.ogg", TranscodeUi::OutputPath("/music/Dummy/roads.flac", {}, false, {}, "ogg"));
  EXPECT_TRUE(TranscodeUi::OutputPath("", "/out", false, {}, "mp3").empty());
}

TEST(TranscodeUi, UniqueOutputPathAndProgress) {
  const std::set<std::string> existing = {"/out/roads.mp3", "/out/roads-1.mp3"};
  EXPECT_EQ("/out/glory.mp3", TranscodeUi::UniqueOutputPath("/out/glory.mp3", [&](const std::string &path) { return existing.count(path) > 0; }));
  EXPECT_EQ("/out/roads-2.mp3", TranscodeUi::UniqueOutputPath("/out/roads.mp3", [&](const std::string &path) { return existing.count(path) > 0; }));
  EXPECT_EQ(0, TranscodeUi::ProgressBarValue(0, 0, 0, {}));
  EXPECT_EQ(200, TranscodeUi::ProgressBarValue(1, 1, 2, {}));
  EXPECT_EQ(150, TranscodeUi::ProgressBarValue(1, 0, 2, {0.5f}));
  EXPECT_EQ(199, TranscodeUi::ProgressBarValue(1, 0, 2, {1.0f}));
  EXPECT_EQ(200, TranscodeUi::ProgressBarMax(2));
  EXPECT_DOUBLE_EQ(0.5, TranscodeUi::ProgressFraction(100, 2));
  EXPECT_EQ("2 remaining, 1 finished, 1 failed", TranscodeUi::StatusText(2, 1, 1));
  EXPECT_TRUE(TranscodeUi::StatusText(0, 0, 0).empty());
  EXPECT_TRUE(TranscodeUi::ShouldShowCancel(true));
  EXPECT_FALSE(TranscodeUi::ShouldShowCancel(false));
  EXPECT_TRUE(TranscodeUi::ShouldContinue(false, 2));
  EXPECT_FALSE(TranscodeUi::ShouldContinue(true, 2));
  EXPECT_FALSE(TranscodeUi::ShouldContinue(false, 0));
}

TEST(TranscodeUi, QueueAudioAndFormatKeys) {
  std::vector<TranscodeUi::QueueItem> files = {{"/music/a.flac", "/music"}};
  EXPECT_TRUE(TranscodeUi::AlreadyQueued(files, "/music/a.flac"));
  EXPECT_FALSE(TranscodeUi::AlreadyQueued(files, "/music/b.flac"));
  EXPECT_TRUE(TranscodeUi::IsAudioPath("/tmp/roads.FLAC"));
  EXPECT_TRUE(TranscodeUi::IsAudioPath("x.opus"));
  EXPECT_FALSE(TranscodeUi::IsAudioPath("/tmp/cover.jpg"));
  EXPECT_FALSE(TranscodeUi::IsAudioPath("/tmp/noext"));
  EXPECT_EQ(3, TranscodeUi::FormatIndexFromKey(TranscoderSettings::kDefaultLastOutputFormat));
  EXPECT_EQ(1, TranscodeUi::FormatIndexFromKey("audio/mp4"));
  EXPECT_EQ(0, TranscodeUi::FormatIndexFromKey("audio/mpeg"));
  EXPECT_STREQ("audio/x-vorbis", TranscodeUi::FormatKey(3));
  EXPECT_STREQ("audio/x-vorbis", TranscodeUi::FormatKey(99));
}

TEST(TranscodeLog, FormatJoinLastAndClear) {
  EXPECT_EQ("Wed Aug 26 15:10:00 2026: Wrote /out/roads.mp3", TranscodeLog::FormatLine("Wed Aug 26 15:10:00 2026", "Wrote /out/roads.mp3"));
  EXPECT_EQ("Wrote /out/roads.mp3", TranscodeLog::FormatLine({}, "Wrote /out/roads.mp3"));
  EXPECT_EQ("Wed Aug 26 15:10:00 2026", TranscodeLog::FormatLine("Wed Aug 26 15:10:00 2026", {}));
  EXPECT_TRUE(TranscodeLog::FormatLine({}, {}).empty());
  const std::string stamp = TranscodeLog::NowStamp();
  EXPECT_FALSE(stamp.empty());
  EXPECT_EQ(stamp + ": Cancelled", TranscodeLog::FormatLine(stamp, "Cancelled"));

  std::vector<std::string> lines;
  TranscodeLog::Append(&lines, TranscodeLog::FormatLine("t1", "Missing source: /tmp/a.flac"));
  TranscodeLog::Append(&lines, TranscodeLog::FormatLine("t2", "Wrote /out/a.mp3"));
  TranscodeLog::Append(&lines, {});
  TranscodeLog::Append(nullptr, "ignored");
  EXPECT_EQ(2u, lines.size());
  EXPECT_EQ("t1: Missing source: /tmp/a.flac\nt2: Wrote /out/a.mp3", TranscodeLog::Join(lines));
  EXPECT_EQ("t2: Wrote /out/a.mp3", TranscodeLog::LastLine(lines));
  TranscodeLog::Clear(&lines);
  TranscodeLog::Clear(nullptr);
  EXPECT_TRUE(lines.empty());
  EXPECT_TRUE(TranscodeLog::Join(lines).empty());
  EXPECT_TRUE(TranscodeLog::LastLine(lines).empty());
}

TEST(Transcoder, CancelClearsQueuedJobs) {
  Transcoder transcoder;
  Song song;
  song.set_url("file:///tmp/roads.flac");
  transcoder.AddJob(song, "/tmp/roads.mp3", Transcoder::Format::MP3);
  EXPECT_EQ(1, transcoder.job_count());
  EXPECT_FALSE(transcoder.cancelled());
  transcoder.Cancel();
  EXPECT_EQ(0, transcoder.job_count());
  EXPECT_TRUE(transcoder.cancelled());
  EXPECT_EQ(0, transcoder.finished_success());
  EXPECT_EQ(0, transcoder.finished_failed());
}
