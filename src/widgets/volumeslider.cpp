#include "widgets/volumeslider.h"

#include <algorithm>

VolumeSlider::VolumeSlider(unsigned max) : StickySlider(0, max, 50) {
  gtk_widget_set_size_request(widget(), 120, -1);
  gtk_widget_set_tooltip_text(widget(), "Volume");
  SetChangedCallback([this](double value) {
    SnapToSticky();
    if (volume_changed_) {
      volume_changed_(static_cast<unsigned>(value));
    }
  });
}

void VolumeSlider::SetEnabled(bool enabled) { gtk_widget_set_sensitive(widget(), enabled); }

void VolumeSlider::SetVolume(unsigned volume) {
  BlockSignals(true);
  set_value(volume);
  BlockSignals(false);
}

unsigned VolumeSlider::volume() const { return static_cast<unsigned>(value()); }

void VolumeSlider::SetVolumeCallback(ChangedCallback callback) { volume_changed_ = std::move(callback); }

void VolumeSlider::HandleWheel(int delta) {
  const double next = std::clamp(value() + (delta > 0 ? 5.0 : -5.0), 0.0, 100.0);
  set_value(next);
}
