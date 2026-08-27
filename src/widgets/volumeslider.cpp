#include "widgets/volumeslider.h"

#include "widgets/volumesliderwheel.h"

#include <algorithm>

VolumeSlider::VolumeSlider(unsigned max) : StickySlider(0, max, 50) {
  gtk_widget_set_size_request(widget(), 120, -1);
  gtk_scale_set_draw_value(GTK_SCALE(widget()), TRUE);
  gtk_scale_set_value_pos(GTK_SCALE(widget()), GTK_POS_TOP);
  gtk_scale_set_format_value_func(
      GTK_SCALE(widget()),
      +[](GtkScale *, double value, gpointer) -> char * { return g_strdup(VolumeSliderWheel::PercentLabel(static_cast<unsigned>(value)).c_str()); },
      nullptr, nullptr);
  UpdatePercent();
  SetChangedCallback([this](double value) {
    SnapToSticky();
    UpdatePercent();
    if (volume_changed_) {
      volume_changed_(static_cast<unsigned>(value));
    }
  });
  GtkEventController *scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(widget(), scroll);
  g_signal_connect(scroll, "scroll", G_CALLBACK((+[](GtkEventControllerScroll *, gdouble, gdouble dy, gpointer data) -> gboolean {
                     static_cast<VolumeSlider *>(data)->HandleGtkScroll(dy);
                     return TRUE;
                   })),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget(), GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble, gdouble, gpointer data) {
                     static_cast<VolumeSlider *>(data)->ShowPresetMenu();
                   }),
                   this);
}

VolumeSlider::~VolumeSlider() {
  if (menu_) {
    gtk_widget_unparent(menu_);
    menu_ = nullptr;
  }
}

void VolumeSlider::SetEnabled(bool enabled) { gtk_widget_set_sensitive(widget(), enabled); }

void VolumeSlider::SetVolume(unsigned volume) {
  BlockSignals(true);
  set_value(volume);
  BlockSignals(false);
  UpdatePercent();
}

unsigned VolumeSlider::volume() const { return static_cast<unsigned>(value()); }

void VolumeSlider::SetVolumeCallback(ChangedCallback callback) { volume_changed_ = std::move(callback); }

void VolumeSlider::UpdatePercent() { gtk_widget_set_tooltip_text(widget(), VolumeSliderWheel::PercentLabel(volume()).c_str()); }

void VolumeSlider::HandleWheel(int delta) {
  const VolumeSliderWheel::Result result = VolumeSliderWheel::FromAngleDelta(wheel_accumulator_, delta);
  wheel_accumulator_ = result.accumulator;
  if (result.steps == 0) {
    return;
  }
  set_value(VolumeSliderWheel::ApplySteps(volume(), result.steps));
}

void VolumeSlider::HandleGtkScroll(double dy) {
  const VolumeSliderWheel::Result result = VolumeSliderWheel::FromGtkScroll(wheel_accumulator_, dy);
  wheel_accumulator_ = result.accumulator;
  if (result.steps == 0) {
    return;
  }
  set_value(VolumeSliderWheel::ApplySteps(volume(), result.steps));
}

void VolumeSlider::ApplyPreset(int percent) { set_value(std::clamp(percent, 0, 100)); }

void VolumeSlider::ShowPresetMenu() {
  if (menu_) {
    gtk_widget_unparent(menu_);
    menu_ = nullptr;
  }
  GMenu *model = g_menu_new();
  for (int preset : VolumeSliderWheel::Presets()) {
    char action[32];
    g_snprintf(action, sizeof(action), "volume.preset(%d)", preset);
    g_menu_append(model, VolumeSliderWheel::PercentLabel(static_cast<unsigned>(preset)).c_str(), action);
  }
  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *preset = g_simple_action_new("preset", G_VARIANT_TYPE_INT32);
  g_signal_connect(preset, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                     static_cast<VolumeSlider *>(data)->ApplyPreset(g_variant_get_int32(param));
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(preset));
  menu_ = gtk_popover_menu_new_from_model(G_MENU_MODEL(model));
  gtk_widget_insert_action_group(menu_, "volume", G_ACTION_GROUP(group));
  gtk_widget_set_parent(menu_, widget());
  gtk_popover_popup(GTK_POPOVER(menu_));
  g_object_unref(group);
  g_object_unref(model);
}
