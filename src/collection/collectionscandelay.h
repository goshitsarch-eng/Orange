#ifndef STRAWBERRY_COLLECTIONSCANDELAY_H
#define STRAWBERRY_COLLECTIONSCANDELAY_H

namespace CollectionScanDelay {

inline constexpr int kRescanMs = 2000;
inline constexpr int kPeriodicMs = 86400 * 1000;

inline bool ShouldArm(bool scanning, bool already_pending) { return !scanning && !already_pending; }

inline bool ShouldRunAfterFinish(bool queued) { return queued; }

}  // namespace CollectionScanDelay

#endif  // STRAWBERRY_COLLECTIONSCANDELAY_H
