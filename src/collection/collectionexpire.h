#ifndef STRAWBERRY_COLLECTIONEXPIRE_H
#define STRAWBERRY_COLLECTIONEXPIRE_H

#include <cstdint>

namespace CollectionExpire {

inline int64_t Cutoff(int64_t now_sec, int expire_days) {
  if (expire_days <= 0) {
    return 0;
  }
  return now_sec - static_cast<int64_t>(expire_days) * 86400;
}

inline bool ShouldExpire(int64_t lastseen, int64_t cutoff, bool unavailable, bool referenced_by_playlist) {
  return unavailable && lastseen > 0 && cutoff > 0 && lastseen < cutoff && !referenced_by_playlist;
}

}  // namespace CollectionExpire

#endif  // STRAWBERRY_COLLECTIONEXPIRE_H
