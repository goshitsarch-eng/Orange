#ifndef STRAWBERRY_LYRICSSEARCHTIMEOUT_H
#define STRAWBERRY_LYRICSSEARCHTIMEOUT_H

#include <cctype>
#include <string>

namespace LyricsSearchTimeout {

constexpr int kEarlyMs = 6000;
constexpr int kHardMs = 15000;
constexpr int kGoodLyricsLength = 60;

inline bool EqualInsensitive(const std::string &left, const std::string &right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (size_t i = 0; i < left.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i]))) {
      return false;
    }
  }
  return true;
}

inline bool ShouldSkipCommercial(const std::string &artist, const std::string &title) {
  return EqualInsensitive(artist, "commercial-free") && EqualInsensitive(title, "listener-supported");
}

inline bool HasUsableResult(bool has_results, int lyrics_length = 0) {
  return has_results && (lyrics_length <= 0 || lyrics_length >= kGoodLyricsLength);
}

inline bool ShouldFinishEarly(int elapsed_ms, bool has_results) { return elapsed_ms >= kEarlyMs && has_results; }

inline bool ShouldFinishHard(int elapsed_ms) { return elapsed_ms >= kHardMs; }

inline bool ShouldFinish(int elapsed_ms, bool has_results) {
  return ShouldFinishHard(elapsed_ms) || ShouldFinishEarly(elapsed_ms, has_results);
}

}  // namespace LyricsSearchTimeout

#endif  // STRAWBERRY_LYRICSSEARCHTIMEOUT_H
