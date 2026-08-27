#include "moodbar/moodbarbuilder.h"

#include "utilities/audioanalysis.h"

std::vector<uint8_t> MoodbarBuilder::FromPcm(const int16_t *samples, size_t count, size_t channels, size_t bins) {
  return AudioAnalysis::MoodFromPeaks(AudioAnalysis::PeaksFromPcm(samples, count, channels, bins));
}
