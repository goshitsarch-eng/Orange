#ifndef STRAWBERRY_TRANSCODERPROGRESS_H
#define STRAWBERRY_TRANSCODERPROGRESS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace TranscoderProgress {

inline constexpr int kProgressIntervalMs = 500;

inline float FractionFromPosition(int64_t position_ns, int64_t duration_ns) {
  if (duration_ns <= 0 || position_ns <= 0) {
    return 0.0f;
  }
  if (position_ns >= duration_ns) {
    return 1.0f;
  }
  return static_cast<float>(position_ns) / static_cast<float>(duration_ns);
}

inline bool ShouldStartNextJob(int current, int queued, int max_threads) {
  return queued > 0 && current < max_threads && max_threads > 0;
}

inline bool AllIdle(int current, int queued) { return current == 0 && queued == 0; }

inline int ClampMaxThreads(int count) { return count < 1 ? 1 : count; }

inline int Remaining(int total, int success, int failed) {
  const int remaining = total - success - failed;
  return remaining > 0 ? remaining : 0;
}

inline std::vector<float> FractionsFromProgress(const std::map<std::string, float> &progress) {
  std::vector<float> fractions;
  fractions.reserve(progress.size());
  for (const auto &item : progress) {
    fractions.push_back(item.second);
  }
  return fractions;
}

}  // namespace TranscoderProgress

#endif  // STRAWBERRY_TRANSCODERPROGRESS_H
