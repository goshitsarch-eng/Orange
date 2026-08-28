#include "moodbar/moodbar.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "moodbar/moodbarpaths.h"
#include "moodbar/moodbarpipeline.h"
#include "utilities/analysisasync.h"
#include "utilities/fileutils.h"
#include "utilities/seekbaranalysis.h"

#include <glib.h>

#include <memory>

namespace {

std::vector<uint8_t> ReadMood(const std::string &path) {
  if (path.empty() || !FileUtils::Exists(path)) {
    return {};
  }
  const std::string data = FileUtils::ReadFile(path);
  return std::vector<uint8_t>(data.begin(), data.end());
}

struct MoodJob {
  MoodbarController *controller = nullptr;
  std::shared_ptr<bool> alive;
  int generation = 0;
  Song song;
  std::string cache_dir;
  std::vector<uint8_t> data;
};

gpointer MoodbarGenerateThread(gpointer data) {
  auto *job = static_cast<MoodJob *>(data);
  job->data = MoodbarLoader().Generate(job->song, false, job->cache_dir);
  g_idle_add(+[](gpointer idle_data) -> gboolean {
    std::unique_ptr<MoodJob> job(static_cast<MoodJob *>(idle_data));
    if (!job->alive || !*job->alive || !job->controller) {
      return G_SOURCE_REMOVE;
    }
    job->controller->ApplyGenerated(job->generation, std::move(job->data), job->song.url());
    return G_SOURCE_REMOVE;
  }, job);
  return nullptr;
}

}  // namespace

std::vector<uint8_t> MoodbarLoader::LoadCached(const Song &song) const {
  const std::string path = FileUtils::PathFromUri(song.url());
  for (const std::string &sidecar : MoodbarPaths::Sidecars(path)) {
    const std::vector<uint8_t> data = ReadMood(sidecar);
    if (!data.empty()) {
      return data;
    }
  }
  return ReadMood(MoodbarPaths::CacheFile(StandardPaths::MoodbarCacheDir(), song.url()));
}

std::vector<uint8_t> MoodbarLoader::Generate(const Song &song, bool save, const std::string &cache_dir) const {
  const std::vector<uint8_t> mood = MoodbarPipeline::Run(song.url());
  if (mood.empty()) {
    return mood;
  }
  const std::string cache = MoodbarPaths::CacheFile(cache_dir.empty() ? StandardPaths::MoodbarCacheDir() : cache_dir, song.url());
  FileUtils::WriteFile(cache, std::string(mood.begin(), mood.end()));
  const std::string path = FileUtils::PathFromUri(song.url());
  if (save && !path.empty()) {
    FileUtils::WriteFile(MoodbarPaths::HiddenSidecar(path), std::string(mood.begin(), mood.end()));
  }
  return mood;
}

std::vector<uint8_t> MoodbarLoader::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  const bool save = settings.BoolValue(MoodbarSettings::kSave, MoodbarSettings::kDefaultSave);
  const std::vector<uint8_t> cached = LoadCached(song);
  if (!cached.empty()) {
    return cached;
  }
  return Generate(song, save, StandardPaths::MoodbarCacheDir());
}

void MoodbarLoader::WriteSidecar(const std::string &url, const std::vector<uint8_t> &mood) const {
  const std::string path = FileUtils::PathFromUri(url);
  if (!SeekbarAnalysis::ShouldWriteSidecar(true, path, !mood.empty())) {
    return;
  }
  FileUtils::WriteFile(MoodbarPaths::HiddenSidecar(path), std::string(mood.begin(), mood.end()));
}

MoodbarController::MoodbarController(MoodbarLoader *loader) : loader_(loader) { ReloadSettings(); }

MoodbarController::~MoodbarController() { *alive_ = false; }

void MoodbarController::ApplyGenerated(int generation, std::vector<uint8_t> data) {
  ApplyGenerated(generation, std::move(data), current_song_.url());
}

void MoodbarController::ApplyGenerated(int generation, std::vector<uint8_t> data, const std::string &url) {
  if (!SeekbarAnalysis::AcceptResult(enabled_, playback_active_, url, current_song_.url(), generation, generation_, alive_ && *alive_)) {
    return;
  }
  data_ = std::move(data);
  busy_ = false;
  if (loader_ && SeekbarAnalysis::ShouldWriteSidecar(save_, FileUtils::PathFromUri(url), !data_.empty())) {
    loader_->WriteSidecar(url, data_);
  }
  Ready.Emit(data_);
}

void MoodbarController::Load(const Song &song) { CurrentSongChanged(song); }

void MoodbarController::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  save_ = settings.BoolValue(MoodbarSettings::kSave, MoodbarSettings::kDefaultSave);
  settings.EndGroup();
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  const bool enabled = static_cast<SeekbarSettings::Mode>(settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode))) ==
                       SeekbarSettings::Mode::Moodbar;
  settings.EndGroup();
  const bool was_enabled = enabled_;
  enabled_ = enabled;
  ApplyEnabledTransition(was_enabled);
}

void MoodbarController::SetEnabled(bool enabled) {
  if (enabled == enabled_) {
    return;
  }
  const bool was_enabled = enabled_;
  enabled_ = enabled;
  ApplyEnabledTransition(was_enabled);
}

void MoodbarController::ApplyEnabledTransition(bool was_enabled) {
  if (SeekbarAnalysis::ShouldGenerateOnEnable(enabled_, was_enabled, current_song_.url())) {
    Generate(current_song_);
  } else if (SeekbarAnalysis::ShouldClearOnDisable(enabled_, was_enabled)) {
    ++generation_;
    busy_ = false;
    data_.clear();
    Ready.Emit(data_);
  }
}

void MoodbarController::CurrentSongChanged(const Song &song) {
  current_song_ = song;
  playback_active_ = true;
  if (!SeekbarAnalysis::ShouldGenerate(enabled_, song)) {
    return;
  }
  Generate(song);
}

void MoodbarController::PlaybackStopped() {
  current_song_ = Song();
  playback_active_ = false;
  ++generation_;
  busy_ = false;
  if (SeekbarAnalysis::ShouldClearOnStop(enabled_)) {
    data_.clear();
    Ready.Emit(data_);
  }
}

void MoodbarController::Generate(const Song &song) {
  ++generation_;
  busy_ = false;
  if (!SeekbarAnalysis::CanLoad(song)) {
    data_.clear();
    Ready.Emit(data_);
    return;
  }
  const std::vector<uint8_t> cached = loader_ ? loader_->LoadCached(song) : std::vector<uint8_t>{};
  if (!AnalysisAsync::NeedsGenerate(true, !cached.empty())) {
    data_ = cached;
    Ready.Emit(data_);
    return;
  }
  data_.clear();
  Ready.Emit(data_);
  if (!loader_) {
    return;
  }
  auto *job = new MoodJob;
  job->controller = this;
  job->alive = alive_;
  job->generation = generation_;
  job->song = song;
  job->cache_dir = StandardPaths::MoodbarCacheDir();
  busy_ = true;
  g_thread_unref(g_thread_new("moodbar-generate", MoodbarGenerateThread, job));
}
