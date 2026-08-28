#ifndef STRAWBERRY_BUSYINDICATORANIM_H
#define STRAWBERRY_BUSYINDICATORANIM_H

namespace BusyIndicatorAnim {

// Qt BusyIndicator starts the movie in showEvent and stops it in hideEvent.
inline bool ShouldStartOnShow() { return true; }

inline bool ShouldStopOnHide() { return true; }

}  // namespace BusyIndicatorAnim

#endif
