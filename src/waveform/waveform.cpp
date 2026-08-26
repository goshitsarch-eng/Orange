#include "waveform/waveform.h"

#include "constants/waveformsettings.h"
#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "utilities/fileutils.h"
#include "waveform/waveformpipeline.h"
#include "waveform/waveformstyle.h"

#include <cstdlib>

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

}  // namespace

std::vector<float> WaveformLoader::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(WaveformSettings::kSettingsGroup);
  const bool save = settings.BoolValue(WaveformSettings::kSave, WaveformSettings::kDefaultSave);
  const std::string path = FileUtils::PathFromUri(song.url());
  for (const std::string &sidecar : WaveformStyle::Sidecars(path)) {
    const std::vector<float> data = ReadPeaks(sidecar);
    if (!data.empty()) {
      return data;
    }
  }
  const std::string cache = WaveformStyle::CacheFile(StandardPaths::WaveformCacheDir(), song.url());
  const std::vector<float> cached = ReadPeaks(cache);
  if (!cached.empty()) {
    return cached;
  }
  const std::vector<float> peaks = WaveformPipeline::Run(song.url());
  if (peaks.empty()) {
    return peaks;
  }
  const std::string serialized = SerializePeaks(peaks);
  FileUtils::WriteFile(cache, serialized);
  if (save && !path.empty()) {
    FileUtils::WriteFile(WaveformStyle::HiddenSidecar(path), serialized);
  }
  return peaks;
}

WaveformController::WaveformController(WaveformLoader *loader) : loader_(loader) {}

void WaveformController::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  if (static_cast<SeekbarSettings::Mode>(settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode))) !=
      SeekbarSettings::Mode::Waveform) {
    data_.clear();
    Ready.Emit(data_);
    return;
  }
  data_ = loader_ ? loader_->Load(song) : std::vector<float>{};
  Ready.Emit(data_);
}
