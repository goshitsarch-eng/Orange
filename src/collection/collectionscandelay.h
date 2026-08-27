#ifndef STRAWBERRY_COLLECTIONSCANDELAY_H
#define STRAWBERRY_COLLECTIONSCANDELAY_H

namespace CollectionScanDelay {

inline constexpr int kRescanMs = 2000;
inline constexpr int kPeriodicMs = 86400 * 1000;

inline bool ShouldArm(bool scanning, bool already_pending, bool paused = false) {
  return !scanning && !already_pending && !paused;
}

inline bool ShouldRunAfterFinish(bool queued) { return queued; }

}  // namespace CollectionScanDelay

#endif  // STRAWBERRY_COLLECTIONSCANDELAY_H
