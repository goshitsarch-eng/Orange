#include "waveform/waveform.h"

#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "utilities/fileutils.h"
#include "waveform/waveformpipeline.h"

#include <cstdlib>

std::vector<float> WaveformLoader::Load(const Song &song) {
  const std::string cache = FileUtils::Join(StandardPaths::WaveformCacheDir(), FileUtils::BaseName(song.url()) + ".wave");
  if (FileUtils::Exists(cache)) {
    const std::string data = FileUtils::ReadFile(cache);
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
    if (!peaks.empty()) {
      return peaks;
    }
  }
  const std::vector<float> peaks = WaveformPipeline::Run(song.url());
  std::string serialized;
  serialized.reserve(peaks.size() * 8);
  for (float peak : peaks) {
    serialized += std::to_string(peak) + "\n";
  }
  FileUtils::WriteFile(cache, serialized);
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
