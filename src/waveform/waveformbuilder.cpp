#include "waveform/waveformbuilder.h"

#include "utilities/audioanalysis.h"

std::vector<float> WaveformBuilder::FromPcm(const int16_t *samples, size_t count, size_t channels, size_t bins) {
  return AudioAnalysis::PeaksFromPcm(samples, count, channels, bins);
}
