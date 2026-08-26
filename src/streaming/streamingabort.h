#ifndef STRAWBERRY_STREAMINGABORT_H
#define STRAWBERRY_STREAMINGABORT_H

namespace StreamingAbort {

inline const char *AbortLabel() { return "Abort"; }

inline bool ShouldShowAbort(bool working) { return working; }

inline bool ShouldShowClose(bool working, bool has_error) { return !working && has_error; }

inline int NextGeneration(int current) { return current + 1; }

inline bool IsCurrent(int generation, int current) { return generation == current; }

}  // namespace StreamingAbort

#endif
