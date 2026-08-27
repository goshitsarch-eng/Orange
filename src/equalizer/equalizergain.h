#ifndef STRAWBERRY_EQUALIZERGAIN_H
#define STRAWBERRY_EQUALIZERGAIN_H

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace EqualizerGain {

inline constexpr int kBandCount = 10;
inline constexpr int kDummyBandCount = 2;
inline constexpr int kPipelineBandCount = kBandCount + kDummyBandCount;
inline constexpr int kSliderMin = -100;
inline constexpr int kSliderMax = 100;
inline constexpr std::array<int, kBandCount> kBandFrequencies = {60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000};

struct BandSetup {
  int child_index = 0;
  float freq = 0.0f;
  float bandwidth = 0.0f;
};

inline int ClampSlider(int slider) { return std::clamp(slider, kSliderMin, kSliderMax); }

inline int PipelineBandIndex(int ui_band) { return ui_band + 1; }

// Qt GstEnginePipeline::UpdateEqualizer maps slider units to dB with a 2:1 negative/positive ratio.
inline float BandGainDb(int slider) {
  slider = ClampSlider(slider);
  return slider < 0 ? static_cast<float>(slider) * 0.24f : static_cast<float>(slider) * 0.12f;
}

inline std::string BandDbLabel(int slider) {
  const float db = BandGainDb(slider);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%g dB", static_cast<double>(db));
  return buf;
}

inline int BandFrequencyHz(int ui_band) {
  if (ui_band < 0 || ui_band >= kBandCount) {
    return 0;
  }
  return kBandFrequencies[static_cast<size_t>(ui_band)];
}

inline float BandBandwidthHz(int ui_band) {
  const int freq = BandFrequencyHz(ui_band);
  const int prev = ui_band <= 0 ? 0 : BandFrequencyHz(ui_band - 1);
  return static_cast<float>(freq - prev);
}

inline BandSetup DummyFirstBand() { return {0, 20.0f, 0.0f}; }

inline BandSetup DummyLastBand() { return {kBandCount + 1, 20000.0f, 0.0f}; }

inline BandSetup UiBandSetup(int ui_band) {
  return {PipelineBandIndex(ui_band), static_cast<float>(BandFrequencyHz(ui_band)), BandBandwidthHz(ui_band)};
}

inline std::array<BandSetup, kPipelineBandCount> AllBandSetups() {
  std::array<BandSetup, kPipelineBandCount> bands{};
  bands[0] = DummyFirstBand();
  for (int i = 0; i < kBandCount; ++i) {
    bands[static_cast<size_t>(PipelineBandIndex(i))] = UiBandSetup(i);
  }
  bands[static_cast<size_t>(kBandCount + 1)] = DummyLastBand();
  return bands;
}

inline std::vector<float> PipelineGains(const std::vector<int> &sliders) {
  std::vector<float> gains;
  gains.reserve(sliders.size());
  for (const int slider : sliders) {
    gains.push_back(BandGainDb(slider));
  }
  return gains;
}

}  // namespace EqualizerGain

#endif
