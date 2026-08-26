#include "moodbar/moodbar.h"

#include "core/standardpaths.h"
#include "utilities/audioanalysis.h"
#include "utilities/fileutils.h"

std::vector<uint8_t> MoodbarLoader::Load(const Song &song) {
  const std::string cache = FileUtils::Join(StandardPaths::MoodbarCacheDir(), FileUtils::BaseName(song.url()) + ".mood");
  if (FileUtils::Exists(cache)) {
    const std::string data = FileUtils::ReadFile(cache);
    if (!data.empty()) {
      return std::vector<uint8_t>(data.begin(), data.end());
    }
  }
  const std::vector<int16_t> pcm = AudioAnalysis::DecodePcm(song.url());
  if (pcm.empty()) {
    return {};
  }
  const std::vector<uint8_t> mood = AudioAnalysis::MoodFromPeaks(AudioAnalysis::PeaksFromPcm(pcm.data(), pcm.size(), 1, 300));
  FileUtils::WriteFile(cache, std::string(mood.begin(), mood.end()));
  return mood;
}

MoodbarController::MoodbarController(MoodbarLoader *loader) : loader_(loader) {}

void MoodbarController::Load(const Song &song) {
  data_ = loader_ ? loader_->Load(song) : std::vector<uint8_t>{};
  Ready.Emit(data_);
}
