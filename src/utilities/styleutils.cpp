#include "utilities/styleutils.h"

#include <adwaita.h>

namespace StyleUtils {

void LoadCss(const std::string &css) {
  GtkCssProvider *provider = gtk_css_provider_new();
#if GTK_CHECK_VERSION(4, 12, 0)
  gtk_css_provider_load_from_string(provider, css.c_str());
#else
  gtk_css_provider_load_from_data(provider, css.c_str(), static_cast<gssize>(css.size()));
#endif
  GdkDisplay *display = gdk_display_get_default();
  if (display) {
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  g_object_unref(provider);
}

bool IsDarkTheme() {
  AdwStyleManager *manager = adw_style_manager_get_default();
  return manager && adw_style_manager_get_dark(manager);
}

}  // namespace StyleUtils
