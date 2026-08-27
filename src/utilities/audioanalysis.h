#ifndef STRAWBERRY_AUDIOANALYSIS_H
#define STRAWBERRY_AUDIOANALYSIS_H

#include <cstdint>
#include <string>
#include <vector>

namespace AudioAnalysis {

std::vector<float> PeaksFromPcm(const int16_t *samples, size_t count, size_t channels, size_t bins);
std::vector<uint8_t> MoodFromPeaks(const std::vector<float> &peaks);
std::vector<int16_t> ScopeFromMagnitudes(const std::vector<float> &db);
std::vector<int16_t> DecodePcm(const std::string &url, size_t max_samples = 165375);

}  // namespace AudioAnalysis

#endif  // STRAWBERRY_AUDIOANALYSIS_H
