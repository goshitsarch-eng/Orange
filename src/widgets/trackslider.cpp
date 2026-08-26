#include "widgets/trackslider.h"

#include "core/settings.h"
#include "widgets/trackslidertime.h"
#include "widgets/tracksliderwheel.h"

TrackSlider::TrackSlider() : slider_(0, 1000, 1) {
  Settings settings;
  settings.BeginGroup(TrackSliderTime::SettingsGroup());
  show_remaining_ = settings.BoolValue(TrackSliderTime::SettingsKey(), TrackSliderTime::DefaultShowRemaining());
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  position_label_ = gtk_label_new("0:00");
  duration_label_ = gtk_label_new("0:00");
  gtk_widget_add_css_class(position_label_, "dim-label");
  gtk_widget_add_css_class(duration_label_, "dim-label");
  gtk_widget_set_tooltip_text(duration_label_, TrackSliderTime::DurationTooltip());
  gtk_widget_set_cursor_from_name(duration_label_, "pointer");
  gtk_widget_set_hexpand(slider_.widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), position_label_);
  gtk_box_append(GTK_BOX(widget_), slider_.widget());
  gtk_box_append(GTK_BOX(widget_), duration_label_);
  GtkGesture *click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(duration_label_, GTK_EVENT_CONTROLLER(click));
  g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint n_press, gdouble, gdouble, gpointer data) {
                     if (n_press == 1) {
                       static_cast<TrackSlider *>(data)->ToggleRemaining();
                     }
                   }),
                   this);
  slider_.SetChangedCallback([this](double value) {
    if (length_nanosec_ <= 0 || !seek_) {
      return;
    }
    seek_(static_cast<int64_t>(value / 1000.0 * static_cast<double>(length_nanosec_)));
  });
  GtkEventController *scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(slider_.widget(), scroll);
  g_signal_connect(scroll, "scroll", G_CALLBACK((+[](GtkEventControllerScroll *, gdouble, gdouble dy, gpointer data) -> gboolean {
                     static_cast<TrackSlider *>(data)->OnWheel(dy);
                     return TRUE;
                   })),
                   this);
  GtkEventController *motion = gtk_event_controller_motion_new();
  gtk_widget_add_controller(slider_.widget(), motion);
  g_signal_connect(motion, "motion", G_CALLBACK(+[](GtkEventControllerMotion *, gdouble x, gdouble, gpointer data) {
                     static_cast<TrackSlider *>(data)->OnHover(x);
                   }),
                   this);
  g_signal_connect(motion, "leave", G_CALLBACK(+[](GtkEventControllerMotion *, gpointer data) {
                     static_cast<TrackSlider *>(data)->HideHover();
                   }),
                   this);
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

void TrackSlider::SetSeekStepCallbacks(StepCallback backward, StepCallback forward) {
  seek_backward_ = std::move(backward);
  seek_forward_ = std::move(forward);
}

void TrackSlider::OnWheel(double dy) {
  const TrackSliderWheel::Result result = TrackSliderWheel::FromGtkScroll(wheel_accumulator_, dy);
  wheel_accumulator_ = result.accumulator;
  const TrackSliderWheel::Direction direction = TrackSliderWheel::DirectionFromSteps(result.steps);
  if (direction == TrackSliderWheel::Direction::Forward && seek_forward_) {
    seek_forward_();
  } else if (direction == TrackSliderWheel::Direction::Backward && seek_backward_) {
    seek_backward_();
  }
}

void TrackSlider::OnHover(double x) { ShowHoverAt(slider_.widget(), x, gtk_widget_get_width(slider_.widget())); }

void TrackSlider::ShowHoverAt(GtkWidget *relative, double x, int width) {
  const int length_sec = static_cast<int>(length_nanosec_ / 1000000000LL);
  const int position_sec = static_cast<int>(position_nanosec_ / 1000000000LL);
  const int hover_sec = TrackSliderHover::SecondsAtX(x, static_cast<double>(width), length_sec);
  popup_.ShowAt(relative, static_cast<int>(x), TrackSliderHover::HoverText(hover_sec), TrackSliderHover::DeltaText(hover_sec, position_sec));
}

void TrackSlider::HideHover() { popup_.Hide(); }

void TrackSlider::SetSliderVisible(bool visible) { gtk_widget_set_visible(slider_.widget(), visible); }

void TrackSlider::ToggleRemaining() {
  show_remaining_ = !show_remaining_;
  PersistRemaining();
  UpdateLabels();
}

void TrackSlider::PersistRemaining() {
  Settings settings;
  settings.BeginGroup(TrackSliderTime::SettingsGroup());
  settings.SetBoolValue(TrackSliderTime::SettingsKey(), show_remaining_);
}

void TrackSlider::UpdateLabels() {
  position_text_ = TrackSliderTime::PositionLabel(position_nanosec_);
  duration_text_ = TrackSliderTime::DurationLabel(show_remaining_, position_nanosec_, length_nanosec_);
  gtk_label_set_text(GTK_LABEL(position_label_), position_text_.c_str());
  gtk_label_set_text(GTK_LABEL(duration_label_), duration_text_.c_str());
  slider_.SetPopupText(TrackSliderTime::PopupText(show_remaining_, position_nanosec_, length_nanosec_));
}
