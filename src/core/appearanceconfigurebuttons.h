#ifndef STRAWBERRY_APPEARANCECONFIGUREBUTTONS_H
#define STRAWBERRY_APPEARANCECONFIGUREBUTTONS_H

#include "constants/appearancesettings.h"
#include "core/appearancecolors.h"
#include "core/settings.h"

#include <gtk/gtk.h>

namespace AppearanceConfigureButtons {

// Qt CollectionFilterWidget::ReloadSettings and StreamingSearchView::ReloadSettings apply kIconSizeConfigureButtons.

enum class Target {
  CollectionOptions,
  CollectionSearch,
  StreamingSearch,
  StreamingConfigure,
  StreamingCollectionFilter,
};

inline const char *CssClass() { return "strawberry-configure-buttons"; }

inline bool ShouldApply(Target target) {
  switch (target) {
    case Target::CollectionOptions:
    case Target::CollectionSearch:
    case Target::StreamingSearch:
    case Target::StreamingConfigure:
    case Target::StreamingCollectionFilter:
      return true;
  }
  return false;
}

inline int IconSize(int stored) {
  return AppearanceColors::ClampIcon(stored, AppearanceSettings::kDefaultIconSizeConfigureButtons);
}

inline int StoredSize() {
  Settings settings;
  settings.BeginGroup(AppearanceSettings::kSettingsGroup);
  return IconSize(settings.IntValue(AppearanceSettings::kIconSizeConfigureButtons, AppearanceSettings::kDefaultIconSizeConfigureButtons));
}

inline void ApplyWidget(GtkWidget *widget, int size) {
  if (!widget) {
    return;
  }
  gtk_widget_add_css_class(widget, CssClass());
  GtkWidget *image = nullptr;
  if (GTK_IS_IMAGE(widget)) {
    image = widget;
  } else if (GTK_IS_BUTTON(widget)) {
    image = gtk_button_get_child(GTK_BUTTON(widget));
  } else if (GTK_IS_MENU_BUTTON(widget) || GTK_IS_SEARCH_ENTRY(widget) || GTK_IS_ENTRY(widget)) {
    image = gtk_widget_get_first_child(widget);
  }
  if (image && !GTK_IS_IMAGE(image)) {
    for (GtkWidget *child = gtk_widget_get_first_child(image); child; child = gtk_widget_get_next_sibling(child)) {
      if (GTK_IS_IMAGE(child)) {
        image = child;
        break;
      }
    }
  }
  if (image && GTK_IS_IMAGE(image)) {
    gtk_image_set_pixel_size(GTK_IMAGE(image), size);
  }
}

}  // namespace AppearanceConfigureButtons

#endif
