#include "moodbar/moodbar.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "moodbar/moodbarpaths.h"
#include "moodbar/moodbarpipeline.h"
#include "utilities/fileutils.h"

namespace {

std::vector<uint8_t> ReadMood(const std::string &path) {
  if (path.empty() || !FileUtils::Exists(path)) {
    return {};
  }
  const std::string data = FileUtils::ReadFile(path);
  return std::vector<uint8_t>(data.begin(), data.end());
}

}  // namespace

std::vector<uint8_t> MoodbarLoader::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  const bool save = settings.BoolValue(MoodbarSettings::kSave, MoodbarSettings::kDefaultSave);
  const std::string path = FileUtils::PathFromUri(song.url());
  for (const std::string &sidecar : MoodbarPaths::Sidecars(path)) {
    const std::vector<uint8_t> data = ReadMood(sidecar);
    if (!data.empty()) {
      return data;
    }
  }
  const std::string cache = MoodbarPaths::CacheFile(StandardPaths::MoodbarCacheDir(), song.url());
  const std::vector<uint8_t> cached = ReadMood(cache);
  if (!cached.empty()) {
    return cached;
  }
  const std::vector<uint8_t> mood = MoodbarPipeline::Run(song.url());
  if (mood.empty()) {
    return mood;
  }
  FileUtils::WriteFile(cache, std::string(mood.begin(), mood.end()));
  if (save && !path.empty()) {
    FileUtils::WriteFile(MoodbarPaths::HiddenSidecar(path), std::string(mood.begin(), mood.end()));
  }
  return mood;
}

MoodbarController::MoodbarController(MoodbarLoader *loader) : loader_(loader) {}

void MoodbarController::Load(const Song &song) {
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  if (static_cast<SeekbarSettings::Mode>(settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode))) !=
      SeekbarSettings::Mode::Moodbar) {
    data_.clear();
    Ready.Emit(data_);
    return;
  }
  data_ = loader_ ? loader_->Load(song) : std::vector<uint8_t>{};
  Ready.Emit(data_);
}
