#include "widgets/tracksliderslider.h"

TrackSliderSlider::TrackSliderSlider() {
  widget_ = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.001);
  gtk_scale_set_draw_value(GTK_SCALE(widget_), FALSE);
  g_signal_connect(widget_, "change-value", G_CALLBACK(+[](GtkRange *range, GtkScrollType, double value, gpointer data) -> gboolean {
                     auto *self = static_cast<TrackSliderSlider *>(data);
                     if (self->seek_) {
                       self->seek_(value);
                     }
                     (void)range;
                     return FALSE;
                   }),
                   this);
}

void TrackSliderSlider::SetValue(double value) { gtk_range_set_value(GTK_RANGE(widget_), value); }

double TrackSliderSlider::Value() const { return gtk_range_get_value(GTK_RANGE(widget_)); }

void TrackSliderSlider::SetSeekCallback(std::function<void(double)> callback) { seek_ = std::move(callback); }
