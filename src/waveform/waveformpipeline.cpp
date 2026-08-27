#include "waveform/waveformpipeline.h"

#include "utilities/audioanalysis.h"
#include "waveform/waveformbuilder.h"

std::vector<float> WaveformPipeline::Run(const std::string &url) {
  const std::vector<int16_t> pcm = AudioAnalysis::DecodePcm(url);
  if (pcm.empty()) {
    return {};
  }
  return WaveformBuilder::FromPcm(pcm.data(), pcm.size());
}
