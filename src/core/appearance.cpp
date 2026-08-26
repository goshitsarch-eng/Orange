#include "core/appearance.h"

#include "constants/appearancesettings.h"
#include "core/settings.h"
#include "utilities/styleutils.h"

#include <adwaita.h>

void Appearance::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(AppearanceSettings::kSettingsGroup);
  dark_mode_ = settings.BoolValue(AppearanceSettings::kDarkMode, AppearanceSettings::kDefaultDarkMode);
  system_icons_ = settings.BoolValue(AppearanceSettings::kSystemThemeIcons, AppearanceSettings::kDefaultSystemIcons);
  style_ = settings.Value(AppearanceSettings::kStyle);
  background_filename_ = settings.Value(AppearanceSettings::kBackgroundImageFilename);
  background_type_ = settings.IntValue(AppearanceSettings::kBackgroundImageType, static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType));
  blur_radius_ = settings.IntValue(AppearanceSettings::kBackgroundImageBlurRadius, AppearanceSettings::kDefaultBackgroundImageBlurRadius);
  opacity_ = settings.IntValue(AppearanceSettings::kBackgroundImageOpacityLevel, AppearanceSettings::kDefaultBackgroundImageOpacityLevel);
}

void Appearance::Apply() {
  ReloadSettings();
  AdwStyleManager *manager = adw_style_manager_get_default();
  if (manager) {
    adw_style_manager_set_color_scheme(manager, dark_mode_ ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT);
  }
  if (!style_.empty()) {
    StyleUtils::LoadCss(style_);
  }
}
