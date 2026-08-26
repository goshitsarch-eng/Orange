#include "settings/appearancesettingspage.h"

#include "constants/appearancesettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *AppearanceSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(AppearanceSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Appearance", "applications-graphics-symbolic");
  AdwPreferencesGroup *theme = SettingsPage::AddGroup(page, "Theme");
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kDarkMode, "Dark mode", nullptr, AppearanceSettings::kDefaultDarkMode);
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kSystemThemeIcons, "Use system icons", nullptr, AppearanceSettings::kDefaultSystemIcons);
  SettingsPage::AddEntry(theme, settings, AppearanceSettings::kStyle, "Style");
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kUseCustomColorSet, "Use a custom color set", nullptr,
                          AppearanceSettings::kDefaultUseCustomColorSet);
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kTabBarSystemColor, "System-colored tab bar", nullptr,
                          AppearanceSettings::kDefaultTabBarSystemColor);
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kTabBarGradient, "Tab bar gradient", nullptr, AppearanceSettings::kDefaultTabBarGradient);

  AdwPreferencesGroup *background = SettingsPage::AddGroup(page, "Background");
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageType, "Background (0 default / 1 none / 2 custom / 3 album / 4 strawbs)",
                            static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType));
  SettingsPage::AddEntry(background, settings, AppearanceSettings::kBackgroundImageFilename, "Custom background file");
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageBlurRadius, "Blur radius",
                            AppearanceSettings::kDefaultBackgroundImageBlurRadius);
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageOpacityLevel, "Opacity",
                            AppearanceSettings::kDefaultBackgroundImageOpacityLevel);
  return page;
}
