#ifndef STRAWBERRY_SETTINGSWHEELTHROUGH_H
#define STRAWBERRY_SETTINGSWHEELTHROUGH_H

#include <algorithm>

#include <gtk/gtk.h>

namespace SettingsWheelThrough {

// Qt SettingsPage::eventFilter / EditTagDialog::eventFilter: Wheel on an unfocused
// combo, spinbox, or slider is forwarded to the parent scroll area.
inline bool ShouldPropagateToParent(bool focused) { return !focused; }

inline bool AppliesToCombo() { return true; }
inline bool AppliesToSpin() { return true; }
inline bool AppliesToSlider() { return true; }

inline constexpr double kFallbackStep = 40.0;

inline double AdjustmentDelta(double dy, double step_increment, double fallback_step = kFallbackStep) {
  return dy * (step_increment > 0.0 ? step_increment : fallback_step);
}

inline double ClampedValue(double current, double delta, double lower, double upper) {
  return std::min(upper, std::max(lower, current + delta));
}

inline GtkWidget *ScrolledAncestor(GtkWidget *widget) {
  return widget ? gtk_widget_get_ancestor(widget, GTK_TYPE_SCROLLED_WINDOW) : nullptr;
}

inline bool ForwardToScrolled(GtkWidget *widget, double dy) {
  GtkWidget *scrolled = ScrolledAncestor(widget);
  if (!scrolled) {
    return true;
  }
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
  if (!adj) {
    return true;
  }
  gtk_adjustment_set_value(adj, ClampedValue(gtk_adjustment_get_value(adj), AdjustmentDelta(dy, gtk_adjustment_get_step_increment(adj)),
                                             gtk_adjustment_get_lower(adj), gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj)));
  return true;
}

inline gboolean OnScroll(GtkEventControllerScroll *controller, double dy) {
  GtkWidget *widget = controller ? gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller)) : nullptr;
  if (!widget || !ShouldPropagateToParent(gtk_widget_has_focus(widget) == TRUE)) {
    return FALSE;
  }
  return ForwardToScrolled(widget, dy) ? TRUE : FALSE;
}

inline void Attach(GtkWidget *widget) {
  if (!widget) {
    return;
  }
  gtk_widget_set_focusable(widget, TRUE);
  gtk_widget_set_focus_on_click(widget, TRUE);
  GtkEventController *scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_event_controller_set_propagation_phase(scroll, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(widget, scroll);
  g_signal_connect(scroll, "scroll", G_CALLBACK((+[](GtkEventControllerScroll *controller, gdouble, gdouble dy, gpointer) -> gboolean {
                     return OnScroll(controller, dy);
                   })),
                   nullptr);
}

}  // namespace SettingsWheelThrough

#endif
