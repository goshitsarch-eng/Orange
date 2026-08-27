#ifndef STRAWBERRY_EQUALIZERCONTROLS_H
#define STRAWBERRY_EQUALIZERCONTROLS_H

namespace EqualizerControls {

// Qt Equalizer::EqualizerEnabledChangedSlot / ReloadSettings — preamp + bands.
inline bool SliderContainerEnabled(bool equalizer_enabled) { return equalizer_enabled; }

// Qt ReloadSettings / StereoBalancerEnabledChangedSlot.
inline bool StereoBalanceSliderEnabled(bool balancer_enabled) { return balancer_enabled; }

inline bool StereoBalanceLabelsEnabled(bool balancer_enabled) { return balancer_enabled; }

// Qt zeros the slider when the balancer is turned off.
inline int DisplayBalanceAfterToggle(bool balancer_enabled, int current) { return balancer_enabled ? current : 0; }

}  // namespace EqualizerControls

#endif
