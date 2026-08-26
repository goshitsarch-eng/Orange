#include "widgets/trackslider.h"

#include "utilities/timeutils.h"

TrackSlider::TrackSlider() : slider_(0, 1000, 1) {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  position_label_ = gtk_label_new("0:00");
  duration_label_ = gtk_label_new("0:00");
  gtk_widget_add_css_class(position_label_, "dim-label");
  gtk_widget_add_css_class(duration_label_, "dim-label");
  gtk_widget_set_hexpand(slider_.widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), position_label_);
  gtk_box_append(GTK_BOX(widget_), slider_.widget());
  gtk_box_append(GTK_BOX(widget_), duration_label_);
  slider_.SetChangedCallback([this](double value) {
    if (length_nanosec_ <= 0 || !seek_) {
      return;
    }
    seek_(static_cast<int64_t>(value / 1000.0 * static_cast<double>(length_nanosec_)));
  });
}

void TrackSlider::SetTimes(int64_t position_nanosec, int64_t length_nanosec) {
  position_nanosec_ = position_nanosec;
  length_nanosec_ = length_nanosec;
  slider_.BlockSignals(true);
  if (length_nanosec_ > 0) {
    slider_.set_value(1000.0 * static_cast<double>(position_nanosec_) / static_cast<double>(length_nanosec_));
  } else {
    slider_.set_value(0);
  }
  slider_.BlockSignals(false);
  UpdateLabels();
}

void TrackSlider::SetSeekCallback(SeekCallback callback) { seek_ = std::move(callback); }

void TrackSlider::UpdateLabels() {
  position_text_ = Utilities::PrettyTimeNanosec(position_nanosec_);
  duration_text_ = Utilities::PrettyTimeNanosec(length_nanosec_);
  gtk_label_set_text(GTK_LABEL(position_label_), position_text_.c_str());
  gtk_label_set_text(GTK_LABEL(duration_label_), duration_text_.c_str());
  slider_.SetPopupText(position_text_ + " / " + duration_text_);
}
