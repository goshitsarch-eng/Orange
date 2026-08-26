#include "moodbar/moodbar.h"

#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"
#include "moodbar/moodbarpipeline.h"
#include "utilities/fileutils.h"

std::vector<uint8_t> MoodbarLoader::Load(const Song &song) {
  const std::string cache = FileUtils::Join(StandardPaths::MoodbarCacheDir(), FileUtils::BaseName(song.url()) + ".mood");
  if (FileUtils::Exists(cache)) {
    const std::string data = FileUtils::ReadFile(cache);
    if (!data.empty()) {
      return std::vector<uint8_t>(data.begin(), data.end());
    }
  }
  const std::vector<uint8_t> mood = MoodbarPipeline::Run(song.url());
  if (!mood.empty()) {
    FileUtils::WriteFile(cache, std::string(mood.begin(), mood.end()));
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
