#include "equalizer/equalizerslider.h"

EqualizerSlider::EqualizerSlider(const std::string &label, int band, int value) : band_(band), value_(value) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  scale_ = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -12, 12, 1);
  gtk_range_set_inverted(GTK_RANGE(scale_), TRUE);
  gtk_range_set_value(GTK_RANGE(scale_), value);
  gtk_widget_set_size_request(scale_, -1, 160);
  g_signal_connect(scale_, "value-changed", G_CALLBACK(OnValueChanged), this);
  gtk_box_append(GTK_BOX(widget_), scale_);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new(label.c_str()));
}

void EqualizerSlider::set_value(int value) {
  value_ = value;
  if (scale_) {
    gtk_range_set_value(GTK_RANGE(scale_), value);
  }
}

void EqualizerSlider::set_changed_callback(ChangedCallback callback, gpointer data) {
  callback_ = callback;
  callback_data_ = data;
}

void EqualizerSlider::OnValueChanged(GtkRange *range, gpointer data) {
  auto *self = static_cast<EqualizerSlider *>(data);
  self->value_ = static_cast<int>(gtk_range_get_value(range));
  if (self->callback_) {
    self->callback_(self->band_, self->value_, self->callback_data_);
  }
}
