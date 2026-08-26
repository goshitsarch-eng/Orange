#ifndef STRAWBERRY_STREAMINGABORT_H
#define STRAWBERRY_STREAMINGABORT_H

#include <string>

namespace StreamingAbort {

inline const char *AbortLabel() { return "Abort"; }

inline const char *CloseLabel() { return "Close"; }

inline bool ShouldShowAbort(bool working) { return working; }

inline bool ShouldShowClose(bool working, bool has_error) { return !working && has_error; }

inline bool ShouldShowAction(bool working, bool has_error) { return ShouldShowAbort(working) || ShouldShowClose(working, has_error); }

inline const char *ButtonLabel(bool working, bool has_error) {
  if (ShouldShowAbort(working)) {
    return AbortLabel();
  }
  (void)has_error;
  return CloseLabel();
}

inline std::string HttpError(unsigned status, const std::string &error) {
  if (!error.empty()) {
    return error;
  }
  if (status == 0) {
    return "Network error";
  }
  return "HTTP " + std::to_string(status);
}

inline int NextGeneration(int current) { return current + 1; }

inline bool IsCurrent(int generation, int current) { return generation == current; }

}  // namespace StreamingAbort

#endif
