#include "moodbar/moodbar.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "moodbar/moodbarpaths.h"
#include "moodbar/moodbarpipeline.h"
#include "utilities/analysisasync.h"
#include "utilities/fileutils.h"

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
  bool save = false;
  std::string cache_dir;
  std::vector<uint8_t> data;
};

gpointer MoodbarGenerateThread(gpointer data) {
  auto *job = static_cast<MoodJob *>(data);
  job->data = MoodbarLoader().Generate(job->song, job->save, job->cache_dir);
  g_idle_add(+[](gpointer idle_data) -> gboolean {
    std::unique_ptr<MoodJob> job(static_cast<MoodJob *>(idle_data));
    if (!job->alive || !*job->alive || !job->controller) {
      return G_SOURCE_REMOVE;
    }
    job->controller->ApplyGenerated(job->generation, std::move(job->data));
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

MoodbarController::MoodbarController(MoodbarLoader *loader) : loader_(loader) {}

MoodbarController::~MoodbarController() { *alive_ = false; }

void MoodbarController::ApplyGenerated(int generation, std::vector<uint8_t> data) {
  if (!AnalysisAsync::AcceptGeneration(generation, generation_, alive_ && *alive_)) {
    return;
  }
  data_ = std::move(data);
  busy_ = false;
  Ready.Emit(data_);
}

void MoodbarController::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  const bool enabled = static_cast<SeekbarSettings::Mode>(settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode))) ==
                       SeekbarSettings::Mode::Moodbar;
  settings.EndGroup();
  ++generation_;
  busy_ = false;
  if (!enabled) {
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
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  auto *job = new MoodJob;
  job->controller = this;
  job->alive = alive_;
  job->generation = generation_;
  job->song = song;
  job->save = settings.BoolValue(MoodbarSettings::kSave, MoodbarSettings::kDefaultSave);
  job->cache_dir = StandardPaths::MoodbarCacheDir();
  busy_ = true;
  g_thread_unref(g_thread_new("moodbar-generate", MoodbarGenerateThread, job));
}
