#ifndef STRAWBERRY_APPEARANCELEFTPANEL_H
#define STRAWBERRY_APPEARANCELEFTPANEL_H

#include "constants/appearancesettings.h"
#include "core/appearancecolors.h"
#include "core/settings.h"

#include <gtk/gtk.h>

namespace AppearanceLeftPanel {

// Qt QueueView / FileView / PlaylistListContainer::ReloadSettings apply kIconSizeLeftPanelButtons.
inline const char *CssClass() { return "strawberry-left-panel-buttons"; }

inline bool ShouldApply() { return true; }

inline bool ShouldReloadOnSettingsClose() { return true; }

inline int IconSize(int stored) { return AppearanceColors::ClampIcon(stored, AppearanceSettings::kDefaultIconSizeLeftPanelButtons); }

inline int StoredSize() {
  Settings settings;
  settings.BeginGroup(AppearanceSettings::kSettingsGroup);
  return IconSize(settings.IntValue(AppearanceSettings::kIconSizeLeftPanelButtons, AppearanceSettings::kDefaultIconSizeLeftPanelButtons));
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

}  // namespace AppearanceLeftPanel

#endif
