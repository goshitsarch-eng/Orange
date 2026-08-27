#ifndef NETWORKTIMEOUTPOLICY_H
#define NETWORKTIMEOUTPOLICY_H

#include <string>

namespace NetworkTimeoutPolicy {

// Qt AcoustidClient / MusicBrainzClient / AlbumCoverFetcherSearch / SubsonicRequest.
inline constexpr int kAcoustidTimeoutMs = 5000;
inline constexpr int kMusicBrainzTimeoutMs = 8000;
inline constexpr int kCoverImageTimeoutMs = 6000;
inline constexpr int kSubsonicTimeoutMs = 30000;

inline std::string TimedOutMessage() { return "Request timed out"; }

inline bool IsCancelled(const std::string &error) {
  if (error.empty()) {
    return false;
  }
  return error.find("cancel") != std::string::npos || error.find("Cancel") != std::string::npos || error == TimedOutMessage();
}

inline std::string FailureMessage(const std::string &error, const std::string &fallback) {
  if (IsCancelled(error)) {
    return TimedOutMessage();
  }
  return error.empty() ? fallback : error;
}

}  // namespace NetworkTimeoutPolicy

#endif  // NETWORKTIMEOUTPOLICY_H
