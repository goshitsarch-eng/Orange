#include "constants/covermanagersettings.h"
#include "constants/edittagdialogsettings.h"
#include "constants/organizesettings.h"
#include "constants/transcodersettings.h"
#include "dialogs/dialoggeometry.h"
#include "core/settings.h"
#include "core/song.h"
#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcodeui.h"
#include "transcoder/transcodelog.h"
#include "transcoder/transcodequality.h"
#include "transcoder/transcoder.h"
#include "transcoder/transcoderprogress.h"

#include <gtest/gtest.h>

#include <map>
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
  EXPECT_EQ(8, TranscodeUi::FormatIndexFromKey("audio/x-wav"));
  EXPECT_EQ(9, TranscodeUi::FormatIndexFromKey("oggflac"));
  EXPECT_EQ(10, TranscodeUi::FormatIndexFromKey("audio/x-alac"));
  EXPECT_STREQ("audio/x-wav", TranscodeUi::FormatKey(8));
  EXPECT_STREQ("audio/x-flac+ogg", TranscodeUi::FormatKey(9));
  EXPECT_STREQ("audio/x-alac", TranscodeUi::FormatKey(10));
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

TEST(TranscodeUi, DestinationAlongsideMatchesQt) {
  EXPECT_STREQ("Transcode Music", TranscodeUi::Title());
  EXPECT_STREQ("Files to transcode", TranscodeUi::FilesGroup());
  EXPECT_STREQ("Add...", TranscodeUi::AddFiles());
  EXPECT_STREQ("Import...", TranscodeUi::Import());
  EXPECT_STREQ("Alongside the originals", TranscodeUi::Alongside());
  EXPECT_STREQ("Filename", TranscodeUi::FilenameColumn());
  EXPECT_STREQ("Directory", TranscodeUi::DirectoryColumn());
  EXPECT_STREQ("Import Directory", TranscodeUi::ImportDirectory());
  EXPECT_STREQ("Progress", TranscodeUi::Progress());
  EXPECT_EQ("roads.flac", TranscodeUi::FilenameOf("/music/Dummy/roads.flac"));
  EXPECT_EQ("/music/Dummy", TranscodeUi::DirectoryOf("/music/Dummy/roads.flac"));
  EXPECT_EQ("roads.flac  —  /music/Dummy", TranscodeUi::QueueRowText({"/music/Dummy/roads.flac", "/music"}));
  EXPECT_EQ("music/Dummy", TranscodeUi::ImportDirectoryOf("/music/Dummy/roads.flac", "/music"));
  EXPECT_EQ("music/Dummy/Sub", TranscodeUi::ImportDirectoryOf("/music/Dummy/Sub/roads.flac", "/music"));
  EXPECT_EQ("music", TranscodeUi::ImportDirectoryOf("/music/roads.flac", "/music"));
  EXPECT_TRUE(TranscodeUi::ImportDirectoryOf("/music/Dummy/roads.flac", {}).empty());
  const auto imported = TranscodeUi::QueueColumns({"/music/Dummy/roads.flac", "/music"});
  ASSERT_EQ(3u, imported.size());
  EXPECT_EQ("roads.flac", imported[0]);
  EXPECT_EQ("/music/Dummy", imported[1]);
  EXPECT_EQ("music/Dummy", imported[2]);
  const auto added = TranscodeUi::QueueColumns({"/music/Dummy/roads.flac", {}});
  ASSERT_EQ(3u, added.size());
  EXPECT_TRUE(added[2].empty());
  EXPECT_FALSE(TranscodeUi::ProgressGroupVisible(false));
  EXPECT_TRUE(TranscodeUi::ProgressGroupVisible(true));
  EXPECT_EQ("720x540", TranscodeUi::EncodeGeometry(720, 540));
  int width = 0;
  int height = 0;
  EXPECT_TRUE(TranscodeUi::DecodeGeometry("800x600", &width, &height));
  EXPECT_EQ(800, width);
  EXPECT_EQ(600, height);
  EXPECT_FALSE(TranscodeUi::DecodeGeometry("50x50", &width, &height));
  EXPECT_FALSE(TranscodeUi::DecodeGeometry("wide", &width, &height));
  EXPECT_STREQ("geometry", TranscoderSettings::kGeometry);
  EXPECT_STREQ("OrganizeDialog", OrganizeSettings::kDialogGroup);
  EXPECT_STREQ("geometry", OrganizeSettings::kGeometry);
  EXPECT_EQ(560, OrganizeSettings::kDefaultDialogWidth);
  EXPECT_STREQ("CoverManager", CoverManagerSettings::kSettingsGroup);
  EXPECT_STREQ("geometry", CoverManagerSettings::kGeometry);
  EXPECT_EQ(860, CoverManagerSettings::kDefaultWidth);
  EXPECT_EQ(680, CoverManagerSettings::kDefaultHeight);
  EXPECT_STREQ("Select...", TranscodeUi::Select());
  EXPECT_STREQ("Preserve directory structure in output directory (import only)", TranscodeUi::Preserve());
  EXPECT_TRUE(TranscodeUi::IsAlongside(0));
  EXPECT_FALSE(TranscodeUi::PreserveSensitive(0));
  EXPECT_TRUE(TranscodeUi::PreserveSensitive(1));
  EXPECT_TRUE(TranscodeUi::DestinationPath({}, 0).empty());
  EXPECT_EQ(0, TranscodeUi::DestinationIndex({}, "/music"));
  std::vector<std::string> folders = TranscodeUi::AddDestinationFolder({}, "/music/out");
  EXPECT_EQ(1u, folders.size());
  EXPECT_EQ(1, TranscodeUi::DestinationIndex(folders, "/music/out"));
  EXPECT_EQ("/music/out", TranscodeUi::DestinationPath(folders, 1));
  folders = TranscodeUi::AddDestinationFolder(folders, "/music/out");
  EXPECT_EQ(1u, folders.size());
  for (int i = 0; i < TranscodeUi::MaxDestinationFolders() + 3; ++i) {
    folders = TranscodeUi::AddDestinationFolder(folders, "/music/out-" + std::to_string(i));
  }
  EXPECT_EQ(TranscodeUi::MaxDestinationFolders(), static_cast<int>(folders.size()));
  EXPECT_EQ("/music/out-3", folders.front());
}

TEST(TranscodeQuality, JobOverrideUsesStoredWhenUnchanged) {
  EXPECT_EQ(-1, TranscodeQuality::JobOverride(5, 5));
  EXPECT_EQ(8, TranscodeQuality::JobOverride(8, 5));
  EXPECT_EQ(2, TranscodeQuality::JobOverride(2, 5));
  Settings settings;
  settings.BeginGroup(TranscoderOptionsFields::GroupFor(Transcoder::Format::FLAC));
  settings.SetIntValue("quality", 2);
  settings.Sync();
  EXPECT_EQ(2, TranscodeQuality::Stored(Transcoder::Format::FLAC));
  EXPECT_EQ(-1, TranscodeQuality::JobOverride(2, TranscodeQuality::Stored(Transcoder::Format::FLAC)));
}

TEST(Transcoder, CancelClearsQueuedJobs) {
  Transcoder transcoder;
  Song song;
  song.set_url("file:///tmp/roads.flac");
  transcoder.AddJob(song, "/tmp/roads.mp3", Transcoder::Format::MP3);
  EXPECT_EQ(1, transcoder.job_count());
  EXPECT_EQ(0, transcoder.current_job_count());
  EXPECT_TRUE(transcoder.GetProgress().empty());
  EXPECT_GE(transcoder.max_threads(), 1);
  EXPECT_FALSE(transcoder.cancelled());
  EXPECT_TRUE(TranscoderProgress::ShouldStartNextJob(transcoder.current_job_count(), transcoder.job_count(), transcoder.max_threads()));
  transcoder.Cancel();
  EXPECT_EQ(0, transcoder.job_count());
  EXPECT_EQ(0, transcoder.current_job_count());
  EXPECT_TRUE(transcoder.cancelled());
  EXPECT_EQ(0, transcoder.finished_success());
  EXPECT_EQ(0, transcoder.finished_failed());
  EXPECT_TRUE(transcoder.GetProgress().empty());
  EXPECT_FALSE(TranscoderProgress::ShouldStartNextJob(transcoder.current_job_count(), transcoder.job_count(), transcoder.max_threads()));
  EXPECT_TRUE(TranscoderProgress::AllIdle(transcoder.current_job_count(), transcoder.job_count()));
}

TEST(TranscoderProgress, FractionStartNextAndRemaining) {
  EXPECT_EQ(500, TranscoderProgress::kProgressIntervalMs);
  EXPECT_FLOAT_EQ(0.0f, TranscoderProgress::FractionFromPosition(0, 0));
  EXPECT_FLOAT_EQ(0.0f, TranscoderProgress::FractionFromPosition(-1, 1000));
  EXPECT_FLOAT_EQ(0.0f, TranscoderProgress::FractionFromPosition(0, 1000));
  EXPECT_FLOAT_EQ(0.5f, TranscoderProgress::FractionFromPosition(500, 1000));
  EXPECT_FLOAT_EQ(1.0f, TranscoderProgress::FractionFromPosition(1000, 1000));
  EXPECT_FLOAT_EQ(1.0f, TranscoderProgress::FractionFromPosition(1500, 1000));
  EXPECT_TRUE(TranscoderProgress::ShouldStartNextJob(0, 1, 4));
  EXPECT_FALSE(TranscoderProgress::ShouldStartNextJob(4, 1, 4));
  EXPECT_FALSE(TranscoderProgress::ShouldStartNextJob(0, 0, 4));
  EXPECT_FALSE(TranscoderProgress::ShouldStartNextJob(0, 1, 0));
  EXPECT_TRUE(TranscoderProgress::AllIdle(0, 0));
  EXPECT_FALSE(TranscoderProgress::AllIdle(1, 0));
  EXPECT_FALSE(TranscoderProgress::AllIdle(0, 1));
  EXPECT_EQ(1, TranscoderProgress::ClampMaxThreads(0));
  EXPECT_EQ(1, TranscoderProgress::ClampMaxThreads(-3));
  EXPECT_EQ(8, TranscoderProgress::ClampMaxThreads(8));
  EXPECT_EQ(2, TranscoderProgress::Remaining(4, 1, 1));
  EXPECT_EQ(0, TranscoderProgress::Remaining(2, 1, 1));
  EXPECT_EQ(0, TranscoderProgress::Remaining(1, 1, 1));
  const std::map<std::string, float> progress = {{"/tmp/a.flac", 0.25f}, {"/tmp/b.flac", 0.5f}};
  const std::vector<float> fractions = TranscoderProgress::FractionsFromProgress(progress);
  EXPECT_EQ(2u, fractions.size());
  EXPECT_EQ(150, TranscodeUi::ProgressBarValue(1, 0, 2, {0.5f}));
}

TEST(DialogGeometry, EncodesAndRestoresLikeTranscodeUi) {
  EXPECT_EQ("720x540", DialogGeometry::Encode(720, 540));
  int width = 0;
  int height = 0;
  EXPECT_TRUE(DialogGeometry::Decode("800x600", &width, &height));
  EXPECT_EQ(800, width);
  EXPECT_EQ(600, height);
  EXPECT_FALSE(DialogGeometry::Decode("50x50", &width, &height));
  EXPECT_FALSE(DialogGeometry::ShouldRestore("wide"));
  EXPECT_TRUE(DialogGeometry::ShouldRestore("640x760"));
  const DialogGeometry::Size restored = DialogGeometry::RestoreOrDefault("", EditTagDialogSettings::kDefaultWidth,
                                                                        EditTagDialogSettings::kDefaultHeight);
  EXPECT_EQ(640, restored.width);
  EXPECT_EQ(760, restored.height);
  const DialogGeometry::Size stored = DialogGeometry::RestoreOrDefault("900x700", 640, 760);
  EXPECT_EQ(900, stored.width);
  EXPECT_EQ(700, stored.height);
}
