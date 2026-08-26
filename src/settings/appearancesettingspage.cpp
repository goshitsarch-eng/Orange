#include "settings/appearancesettingspage.h"

#include "constants/appearancesettings.h"
#include "core/appearancecolors.h"
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
  SettingsPage::AddButtonRow(theme, Translations::CStr("Custom colors"), Translations::CStr("Set dark colors"), [settings]() {
    settings->BeginGroup(AppearanceSettings::kSettingsGroup);
    settings->SetBoolValue(AppearanceSettings::kUseCustomColorSet, true);
    for (const auto &role : AppearanceColors::Roles()) {
      settings->SetValue(role.key, role.dark_hex);
    }
    settings->Sync();
  });
  SettingsPage::AddButtonRow(theme, Translations::CStr("Custom colors"), Translations::CStr("Reset colors"), [settings]() {
    settings->BeginGroup(AppearanceSettings::kSettingsGroup);
    settings->SetBoolValue(AppearanceSettings::kUseCustomColorSet, false);
    for (const auto &role : AppearanceColors::Roles()) {
      settings->Remove(role.key);
    }
    settings->Sync();
  });

  AdwPreferencesGroup *colors = SettingsPage::AddGroup(page, "Custom colors");
  for (const auto &role : AppearanceColors::Roles()) {
    SettingsPage::AddColorButton(colors, settings, AppearanceSettings::kSettingsGroup, role.key, role.title, role.dark_hex);
  }

  AdwPreferencesGroup *tabbar = SettingsPage::AddGroup(page, "Tab bar");
  SettingsPage::AddToggle(tabbar, settings, AppearanceSettings::kTabBarSystemColor, "System-colored tab bar", nullptr,
                          AppearanceSettings::kDefaultTabBarSystemColor);
  SettingsPage::AddToggle(tabbar, settings, AppearanceSettings::kTabBarGradient, "Tab bar gradient", nullptr, AppearanceSettings::kDefaultTabBarGradient);
  SettingsPage::AddColorButton(tabbar, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kTabBarColor, "Tab bar color",
                              "#404040");

  AdwPreferencesGroup *playlist = SettingsPage::AddGroup(page, "Playlist");
  SettingsPage::AddColorButton(playlist, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kPlaylistPlayingSongColor,
                              "Playlist playing song color", "#6696e3");

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
  SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageStretch, "Stretch background", nullptr,
                          AppearanceSettings::kDefaultBackgroundImageStretch);
  SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageKeepAspectRatio, "Keep aspect ratio", nullptr,
                          AppearanceSettings::kDefaultBackgroundImageKeepAspectRatio);
  SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageDoNotCut, "Do not crop", nullptr,
                          AppearanceSettings::kDefaultBackgroundImageDoNotCut);
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageMaxSize, "Maximum size",
                            AppearanceSettings::kDefaultBackgroundImageMaxSize);
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageBlurRadius, "Blur radius",
                            AppearanceSettings::kDefaultBackgroundImageBlurRadius);
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageOpacityLevel, "Opacity",
                            AppearanceSettings::kDefaultBackgroundImageOpacityLevel);

  AdwPreferencesGroup *icons = SettingsPage::AddGroup(page, "Icon sizes");
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeTabbarSmallMode, "Tab bar (small)",
                            AppearanceSettings::kDefaultIconSizeTabbarSmallMode);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeTabbarLargeMode, "Tab bar (large)",
                            AppearanceSettings::kDefaultIconSizeTabbarLargeMode);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizePlayControlButtons, "Play controls",
                            AppearanceSettings::kDefaultIconSizePlayControlButtons);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizePlaylistButtons, "Playlist buttons",
                            AppearanceSettings::kDefaultIconSizePlaylistButtons);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeLeftPanelButtons, "Left panel buttons",
                            AppearanceSettings::kDefaultIconSizeLeftPanelButtons);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeConfigureButtons, "Configure buttons",
                            AppearanceSettings::kDefaultIconSizeConfigureButtons);
  return page;
}
