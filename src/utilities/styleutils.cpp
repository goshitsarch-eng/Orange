#include "utilities/styleutils.h"

#include <adwaita.h>

#include <map>

namespace StyleUtils {

namespace {

// Providers are intentionally never destroyed: they live as long as the display they are attached to.
// Keyed by slot so that re-loading a slot updates the CSS in place.
GtkCssProvider *ProviderForSlot(const std::string &slot) {
  static std::map<std::string, GtkCssProvider *> providers;
  const auto it = providers.find(slot);
  if (it != providers.end()) {
    return it->second;
  }
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    return nullptr;
  }
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  providers.emplace(slot, provider);
  return provider;
}

void SetSlotCss(const std::string &slot, const std::string &css) {
  GtkCssProvider *provider = ProviderForSlot(slot);
  if (!provider) {
    return;
  }
#if GTK_CHECK_VERSION(4, 12, 0)
  gtk_css_provider_load_from_string(provider, css.c_str());
#else
  gtk_css_provider_load_from_data(provider, css.c_str(), static_cast<gssize>(css.size()));
#endif
}

}  // namespace

void LoadCss(const std::string &css, const std::string &slot) { SetSlotCss(slot, css); }

void ClearCss(const std::string &slot) { SetSlotCss(slot, std::string()); }

bool IsDarkTheme() {
  AdwStyleManager *manager = adw_style_manager_get_default();
  return manager && adw_style_manager_get_dark(manager);
}

}  // namespace StyleUtils
