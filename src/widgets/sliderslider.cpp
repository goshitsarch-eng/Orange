#include "widgets/sliderslider.h"

SliderSlider::SliderSlider(double min, double max, double step) {
  widget_ = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_scale_set_draw_value(GTK_SCALE(widget_), FALSE);
  g_signal_connect(widget_, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     auto *self = static_cast<SliderSlider *>(data);
                     if (!self->blocked_ && self->changed_) {
                       self->changed_(gtk_range_get_value(range));
                     }
                   }),
                   this);
}

double SliderSlider::value() const { return gtk_range_get_value(GTK_RANGE(widget_)); }

void SliderSlider::set_value(double value) { gtk_range_set_value(GTK_RANGE(widget_), value); }

void SliderSlider::SetRange(double min, double max) { gtk_range_set_range(GTK_RANGE(widget_), min, max); }

void SliderSlider::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void SliderSlider::BlockSignals(bool block) { blocked_ = block; }

void SliderSlider::CancelGestures() {
  if (!widget_) {
    return;
  }
  GListModel *controllers = gtk_widget_observe_controllers(widget_);
  if (!controllers) {
    return;
  }
  const guint n = g_list_model_get_n_items(controllers);
  for (guint i = 0; i < n; ++i) {
    auto *controller = GTK_EVENT_CONTROLLER(g_list_model_get_item(controllers, i));
    if (GTK_IS_GESTURE(controller)) {
      gtk_gesture_set_state(GTK_GESTURE(controller), GTK_EVENT_SEQUENCE_DENIED);
    }
    if (controller) {
      g_object_unref(controller);
    }
  }
  g_object_unref(controllers);
}
