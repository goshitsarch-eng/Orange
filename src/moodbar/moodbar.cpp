#include "moodbar/moodbar.h"
#include "config.h"
#include "core/standardpaths.h"
#include "utilities/fileutils.h"
#ifdef HAVE_GSTFASTSPECTRUM
#include <fftw3.h>
#endif
std::vector<uint8_t> MoodbarLoader::Load(const Song &song) {
  const std::string cache = FileUtils::Join(StandardPaths::MoodbarCacheDir(), FileUtils::BaseName(song.url()) + ".mood");
  if (FileUtils::Exists(cache)) {
    const std::string data = FileUtils::ReadFile(cache);
    return std::vector<uint8_t>(data.begin(), data.end());
  }
  return std::vector<uint8_t>(300, 128);
}
MoodbarController::MoodbarController(MoodbarLoader *loader) : loader_(loader) {}
void MoodbarController::Load(const Song &song) {
  data_ = loader_ ? loader_->Load(song) : std::vector<uint8_t>{};
  Ready.Emit(data_);
}
