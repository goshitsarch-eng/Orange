#ifndef STRAWBERRY_VOLUMESLIDER_H
#define STRAWBERRY_VOLUMESLIDER_H

#include "widgets/stickyslider.h"

#include <functional>

class VolumeSlider : public StickySlider {
 public:
  using ChangedCallback = std::function<void(unsigned)>;

  explicit VolumeSlider(unsigned max = 100);
  ~VolumeSlider();

  void SetEnabled(bool enabled);
  void SetVolume(unsigned volume);
  unsigned volume() const;
  void SetVolumeCallback(ChangedCallback callback);
  void HandleWheel(int delta);
  void HandleGtkScroll(double dy);

 private:
  void UpdatePercent();
  void ShowPresetMenu();
  void ApplyPreset(int percent);

  ChangedCallback volume_changed_;
  int wheel_accumulator_ = 0;
  GtkWidget *menu_ = nullptr;
};

#endif
