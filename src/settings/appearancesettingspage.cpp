#include "settings/appearancesettingspage.h"

#include "constants/appearancesettings.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

#include <gio/gio.h>

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
  SettingsPage::AddCombo(background, settings, AppearanceSettings::kBackgroundImageType, "Background",
                         {{"0", "Default"}, {"1", "None"}, {"2", "Custom image"}, {"3", "Album cover"}, {"4", "Strawberry"}},
                         std::to_string(static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType)));
  SettingsPage::AddEntry(background, settings, AppearanceSettings::kBackgroundImageFilename, "Custom background file");
  SettingsPage::AddButtonRow(background, "Custom background file", "Choose image…", [settings]() {
    GtkFileDialog *chooser = gtk_file_dialog_new();
    gtk_file_dialog_set_title(chooser, Translations::CStr("Choose background image"));
    gtk_file_dialog_open(chooser, nullptr, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
      auto *s = static_cast<Settings *>(data);
      GError *error = nullptr;
      GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
      if (!file) {
        if (error) {
          g_error_free(error);
        }
        return;
      }
      gchar *path = g_file_get_path(file);
      if (path && s) {
        s->BeginGroup(AppearanceSettings::kSettingsGroup);
        s->SetValue(AppearanceSettings::kBackgroundImageFilename, path);
        s->SetValue(AppearanceSettings::kBackgroundImageType, "2");
        s->Sync();
      }
      g_free(path);
      g_object_unref(file);
    }, settings);
  });
  SettingsPage::AddCombo(background, settings, AppearanceSettings::kBackgroundImagePosition, "Position",
                         {{"1", "Upper left"}, {"2", "Upper right"}, {"3", "Middle"}, {"4", "Bottom left"}, {"5", "Bottom right"}},
                         std::to_string(static_cast<int>(AppearanceSettings::kDefaultBackgroundImagePosition)));
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageBlurRadius, "Blur radius",
                            AppearanceSettings::kDefaultBackgroundImageBlurRadius);
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageOpacityLevel, "Opacity",
                            AppearanceSettings::kDefaultBackgroundImageOpacityLevel);
  return page;
}
