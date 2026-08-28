#include "waveform/waveform.h"

#include "constants/waveformsettings.h"
#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "utilities/analysisasync.h"
#include "utilities/fileutils.h"
#include "utilities/seekbaranalysis.h"
#include "waveform/waveformpipeline.h"
#include "waveform/waveformstyle.h"

#include <glib.h>

#include <cstdlib>
#include <memory>

namespace {

std::vector<float> ParsePeaks(const std::string &data) {
  std::vector<float> peaks;
  size_t pos = 0;
  while (pos < data.size()) {
    const size_t end = data.find('\n', pos);
    const std::string line = data.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    if (!line.empty()) {
      peaks.push_back(std::strtof(line.c_str(), nullptr));
    }
    if (end == std::string::npos) {
      break;
    }
    pos = end + 1;
  }
  return peaks;
}

std::string SerializePeaks(const std::vector<float> &peaks) {
  std::string serialized;
  serialized.reserve(peaks.size() * 8);
  for (float peak : peaks) {
    serialized += std::to_string(peak) + "\n";
  }
  return serialized;
}

std::vector<float> ReadPeaks(const std::string &path) {
  if (path.empty() || !FileUtils::Exists(path)) {
    return {};
  }
  return ParsePeaks(FileUtils::ReadFile(path));
}

struct WaveJob {
  WaveformController *controller = nullptr;
  std::shared_ptr<bool> alive;
  int generation = 0;
  Song song;
  std::string cache_dir;
  std::vector<float> data;
};

gpointer WaveformGenerateThread(gpointer data) {
  auto *job = static_cast<WaveJob *>(data);
  job->data = WaveformLoader().Generate(job->song, false, job->cache_dir);
  g_idle_add(+[](gpointer idle_data) -> gboolean {
    std::unique_ptr<WaveJob> job(static_cast<WaveJob *>(idle_data));
    if (!job->alive || !*job->alive || !job->controller) {
      return G_SOURCE_REMOVE;
    }
    job->controller->ApplyGenerated(job->generation, std::move(job->data), job->song.url());
    return G_SOURCE_REMOVE;
  }, job);
  return nullptr;
}

}  // namespace

std::vector<float> WaveformLoader::LoadCached(const Song &song) const {
  const std::string path = FileUtils::PathFromUri(song.url());
  for (const std::string &sidecar : WaveformStyle::Sidecars(path)) {
    const std::vector<float> data = ReadPeaks(sidecar);
    if (!data.empty()) {
      return data;
    }
  }
  return ReadPeaks(WaveformStyle::CacheFile(StandardPaths::WaveformCacheDir(), song.url()));
}

std::vector<float> WaveformLoader::Generate(const Song &song, bool save, const std::string &cache_dir) const {
  const std::vector<float> peaks = WaveformPipeline::Run(song.url());
  if (peaks.empty()) {
    return peaks;
  }
  const std::string serialized = SerializePeaks(peaks);
  const std::string cache = WaveformStyle::CacheFile(cache_dir.empty() ? StandardPaths::WaveformCacheDir() : cache_dir, song.url());
  FileUtils::WriteFile(cache, serialized);
  const std::string path = FileUtils::PathFromUri(song.url());
  if (save && !path.empty()) {
    FileUtils::WriteFile(WaveformStyle::HiddenSidecar(path), serialized);
  }
  return peaks;
}

std::vector<float> WaveformLoader::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(WaveformSettings::kSettingsGroup);
  const bool save = settings.BoolValue(WaveformSettings::kSave, WaveformSettings::kDefaultSave);
  const std::vector<float> cached = LoadCached(song);
  if (!cached.empty()) {
    return cached;
  }
  return Generate(song, save, StandardPaths::WaveformCacheDir());
}

void WaveformLoader::WriteSidecar(const std::string &url, const std::vector<float> &peaks) const {
  const std::string path = FileUtils::PathFromUri(url);
  if (!SeekbarAnalysis::ShouldWriteSidecar(true, path, !peaks.empty())) {
    return;
  }
  FileUtils::WriteFile(WaveformStyle::HiddenSidecar(path), SerializePeaks(peaks));
}

WaveformController::WaveformController(WaveformLoader *loader) : loader_(loader) { ReloadSettings(); }

WaveformController::~WaveformController() { *alive_ = false; }

void WaveformController::ApplyGenerated(int generation, std::vector<float> data) {
  ApplyGenerated(generation, std::move(data), current_song_.url());
}

void WaveformController::ApplyGenerated(int generation, std::vector<float> data, const std::string &url) {
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

void WaveformController::Load(const Song &song) { CurrentSongChanged(song); }

void WaveformController::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(WaveformSettings::kSettingsGroup);
  save_ = settings.BoolValue(WaveformSettings::kSave, WaveformSettings::kDefaultSave);
  settings.EndGroup();
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  const bool enabled = static_cast<SeekbarSettings::Mode>(settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode))) ==
                       SeekbarSettings::Mode::Waveform;
  settings.EndGroup();
  const bool was_enabled = enabled_;
  enabled_ = enabled;
  ApplyEnabledTransition(was_enabled);
}

void WaveformController::SetEnabled(bool enabled) {
  if (enabled == enabled_) {
    return;
  }
  const bool was_enabled = enabled_;
  enabled_ = enabled;
  ApplyEnabledTransition(was_enabled);
}

void WaveformController::ApplyEnabledTransition(bool was_enabled) {
  if (SeekbarAnalysis::ShouldGenerateOnEnable(enabled_, was_enabled, current_song_.url())) {
    Generate(current_song_);
  } else if (SeekbarAnalysis::ShouldClearOnDisable(enabled_, was_enabled)) {
    ++generation_;
    busy_ = false;
    data_.clear();
    Ready.Emit(data_);
  }
}

void WaveformController::CurrentSongChanged(const Song &song) {
  current_song_ = song;
  playback_active_ = true;
  if (!SeekbarAnalysis::ShouldGenerate(enabled_, song)) {
    return;
  }
  Generate(song);
}

void WaveformController::PlaybackStopped() {
  current_song_ = Song();
  playback_active_ = false;
  ++generation_;
  busy_ = false;
  if (SeekbarAnalysis::ShouldClearOnStop(enabled_)) {
    data_.clear();
    Ready.Emit(data_);
  }
}

void WaveformController::Generate(const Song &song) {
  ++generation_;
  busy_ = false;
  if (!SeekbarAnalysis::CanLoad(song)) {
    data_.clear();
    Ready.Emit(data_);
    return;
  }
  const std::vector<float> cached = loader_ ? loader_->LoadCached(song) : std::vector<float>{};
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
  auto *job = new WaveJob;
  job->controller = this;
  job->alive = alive_;
  job->generation = generation_;
  job->song = song;
  job->cache_dir = StandardPaths::WaveformCacheDir();
  busy_ = true;
  g_thread_unref(g_thread_new("waveform-generate", WaveformGenerateThread, job));
}
