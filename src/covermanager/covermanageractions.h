#ifndef STRAWBERRY_COVERMANAGERACTIONS_H
#define STRAWBERRY_COVERMANAGERACTIONS_H

namespace CoverManagerActions {

inline const char *WindowTitle() { return "Cover Manager"; }

inline bool DoubleClickShowsCover() { return true; }

inline bool ShowStatisticsWhenFetchFinishes(bool started, bool cancelled, size_t total) {
  return started && !cancelled && total > 0;
}

}  // namespace CoverManagerActions

#endif  // STRAWBERRY_COVERMANAGERACTIONS_H
