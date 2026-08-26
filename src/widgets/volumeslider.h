#ifndef STRAWBERRY_VOLUMESLIDER_H
#define STRAWBERRY_VOLUMESLIDER_H

#include "widgets/stickyslider.h"

#include <functional>

class VolumeSlider : public StickySlider {
 public:
  using ChangedCallback = std::function<void(unsigned)>;

  explicit VolumeSlider(unsigned max = 100);

  void SetEnabled(bool enabled);
  void SetVolume(unsigned volume);
  unsigned volume() const;
  void SetVolumeCallback(ChangedCallback callback);
  void HandleWheel(int delta);

 private:
  ChangedCallback volume_changed_;
};

#endif
