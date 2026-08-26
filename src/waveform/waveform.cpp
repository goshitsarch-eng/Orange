#include "waveform/waveform.h"
#include "core/standardpaths.h"
#include "utilities/fileutils.h"
std::vector<float> WaveformLoader::Load(const Song &song) {
  (void)song;
  return std::vector<float>(512, 0.2f);
}
WaveformController::WaveformController(WaveformLoader *loader) : loader_(loader) {}
void WaveformController::Load(const Song &song) {
  data_ = loader_ ? loader_->Load(song) : std::vector<float>{};
  Ready.Emit(data_);
}
