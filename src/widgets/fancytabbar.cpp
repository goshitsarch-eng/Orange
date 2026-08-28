#include "widgets/fancytabbar.h"

#include "constants/appearancesettings.h"
#include "core/settings.h"
#include "translations/translations.h"

#include <cstring>
#include <pango/pango.h>

FancyTabBar::FancyTabBar() {
  widget_ = gtk_box_new(FancyTabMode::BarOrientation(mode_), 2);
  gtk_widget_add_css_class(widget_, "sidebar");
  gtk_widget_add_css_class(widget_, "strawberry-tabbar");
  gtk_widget_set_focusable(widget_, TRUE);
  ReloadIconSizes();

  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK((+[](GtkGestureClick *gesture, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<FancyTabBar *>(data);
                     if (FancyTabMode::ShouldShowMenu()) {
                       self->ShowModeMenu();
                     }
                     gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
                   })),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget_, keys);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return static_cast<FancyTabBar *>(data)->OnKeyPressed(keyval, state);
                   })),
                   this);
}

void FancyTabBar::ReloadIconSizes() {
  Settings settings;
  settings.BeginGroup(AppearanceSettings::kSettingsGroup);
  icon_large_ = settings.IntValue(AppearanceSettings::kIconSizeTabbarLargeMode, FancyTabMode::kLargeIcon);
  icon_small_ = settings.IntValue(AppearanceSettings::kIconSizeTabbarSmallMode, FancyTabMode::kSmallIcon);
  Rebuild();
}

int FancyTabBar::IconPixels() const { return FancyTabMode::IconSize(mode_, icon_large_, icon_small_); }

int FancyTabBar::ActiveIndex() const {
  for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
    if (tabs_[static_cast<size_t>(i)].id == active_) {
      return i;
    }
  }
  return -1;
}

void FancyTabBar::AddTab(const std::string &id, const std::string &title, const std::string &icon) {
  FancyTabData tab;
  tab.id = id;
  tab.title = title;
  tab.icon = icon;
  tabs_.push_back(tab);
  if (active_.empty()) {
    active_ = id;
  }
  Rebuild();
}

void FancyTabBar::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void FancyTabBar::SetModeChangedCallback(ModeChangedCallback callback) { mode_changed_ = std::move(callback); }

void FancyTabBar::SetMode(FancyTabMode::Mode mode) {
  const FancyTabMode::Mode next = FancyTabMode::FromStored(FancyTabMode::ToStored(mode));
  if (next == mode_) {
    Rebuild();
    return;
  }
  mode_ = next;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(widget_), FancyTabMode::BarOrientation(mode_));
  Rebuild();
  if (mode_changed_) {
    mode_changed_(mode_);
  }
}

void FancyTabBar::SetActive(const std::string &id, bool notify) {
  bool found = false;
  for (const FancyTabData &tab : tabs_) {
    if (tab.id == id) {
      found = true;
      break;
    }
  }
  if (!found) {
    return;
  }
  if (active_ == id) {
    UpdateToggles();
    return;
  }
  active_ = id;
  UpdateToggles();
  if (notify && activate_) {
    activate_(id);
  }
}

void FancyTabBar::SetActiveIndex(int index, bool notify) {
  if (index < 0 || index >= static_cast<int>(tabs_.size())) {
    return;
  }
  SetActive(tabs_[static_cast<size_t>(index)].id, notify);
}

gboolean FancyTabBar::OnKeyPressed(guint keyval, GdkModifierType state) {
  if (!FancyTabMode::IsKeyboardTrigger(keyval, static_cast<unsigned>(state))) {
    return FALSE;
  }
  if (FancyTabMode::ShouldShowMenu()) {
    ShowModeMenu();
  }
  return TRUE;
}

void FancyTabBar::Rebuild() {
  if (!widget_) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(widget_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(widget_), child);
    child = next;
  }
  GtkWidget *group = nullptr;
  for (const FancyTabData &tab : tabs_) {
    GtkWidget *button = gtk_toggle_button_new();
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_hexpand(button, FancyTabMode::IsTop(mode_) ? TRUE : FALSE);
    if (group) {
      gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(button), GTK_TOGGLE_BUTTON(group));
    } else {
      group = button;
    }
    GtkWidget *box = gtk_box_new(FancyTabMode::ButtonOrientation(mode_), 4);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    if (FancyTabMode::ShowsIcon(mode_)) {
      GtkWidget *image = gtk_image_new_from_icon_name(tab.icon.empty() ? "audio-x-generic-symbolic" : tab.icon.c_str());
      gtk_image_set_pixel_size(GTK_IMAGE(image), IconPixels());
      gtk_box_append(GTK_BOX(box), image);
    }
    if (FancyTabMode::ShowsText(mode_)) {
      GtkWidget *label = gtk_label_new(tab.title.c_str());
      gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
      gtk_box_append(GTK_BOX(box), label);
    } else {
      gtk_widget_set_tooltip_text(button, tab.title.c_str());
    }
    gtk_button_set_child(GTK_BUTTON(button), box);
    g_object_set_data_full(G_OBJECT(button), "tab-id", g_strdup(tab.id.c_str()), g_free);
    g_signal_connect(button, "toggled", G_CALLBACK((+[](GtkToggleButton *btn, gpointer data) {
                       if (!gtk_toggle_button_get_active(btn)) {
                         return;
                       }
                       auto *self = static_cast<FancyTabBar *>(data);
                       const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "tab-id"));
                       if (id) {
                         self->SetActive(id);
                       }
                     })),
                     this);
    gtk_box_append(GTK_BOX(widget_), button);
  }
  UpdateToggles();
}

void FancyTabBar::UpdateToggles() const {
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_TOGGLE_BUTTON(child)) {
      continue;
    }
    const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(child), "tab-id"));
    const bool on = id && active_ == id;
    if (static_cast<bool>(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(child))) != on) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(child), on ? TRUE : FALSE);
    }
  }
}

void FancyTabBar::ShowModeMenu() {
  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, widget_);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);
  GtkWidget *group = nullptr;
  for (const FancyTabMode::Item &item : FancyTabMode::MenuItems()) {
    GtkWidget *check = gtk_check_button_new_with_label(Translations::CStr(item.label));
    if (group) {
      gtk_check_button_set_group(GTK_CHECK_BUTTON(check), GTK_CHECK_BUTTON(group));
    } else {
      group = check;
    }
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), item.mode == mode_ ? TRUE : FALSE);
    g_object_set_data(G_OBJECT(check), "mode", GINT_TO_POINTER(FancyTabMode::ToStored(item.mode)));
    g_signal_connect(check, "toggled", G_CALLBACK((+[](GtkCheckButton *button, gpointer data) {
                       if (!gtk_check_button_get_active(button)) {
                         return;
                       }
                       auto *self = static_cast<FancyTabBar *>(data);
                       const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "mode"));
                       self->SetMode(FancyTabMode::FromStored(stored));
                       if (GtkWidget *pop = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_POPOVER)) {
                         gtk_popover_popdown(GTK_POPOVER(pop));
                       }
                     })),
                     this);
    gtk_box_append(GTK_BOX(box), check);
  }
  gtk_popover_set_child(GTK_POPOVER(popover), box);
  gtk_popover_popup(GTK_POPOVER(popover));
}
