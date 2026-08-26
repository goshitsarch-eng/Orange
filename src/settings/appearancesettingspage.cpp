#include "settings/appearancesettingspage.h"

#include "constants/appearancesettings.h"
#include "core/appearancecolors.h"
#include "core/appearancestyle.h"
#include "settings/appearancesettingslabels.h"
#include "settings/settingscontrols.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

#include <gio/gio.h>

AdwPreferencesPage *AppearanceSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(AppearanceSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Appearance", "applications-graphics-symbolic");
  AdwPreferencesGroup *theme = SettingsPage::AddGroup(page, "Theme");
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kDarkMode, AppearanceSettingsLabels::DarkMode(),
                          AppearanceSettingsLabels::DarkRestart(), AppearanceSettings::kDefaultDarkMode);
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kSystemThemeIcons, AppearanceSettingsLabels::SystemIcons(), nullptr,
                          AppearanceSettings::kDefaultSystemIcons);
  SettingsPage::AddCombo(theme, settings, AppearanceSettings::kStyle, "Style", AppearanceStyle::Choices(), "");
  SettingsPage::AddDescription(theme, AppearanceSettingsLabels::StyleRestart());
  SettingsPage::AddBoolRadios(theme, settings, AppearanceSettings::kUseCustomColorSet, AppearanceSettingsLabels::SystemColorSet(),
                             AppearanceSettingsLabels::CustomColorSet(), AppearanceSettings::kDefaultUseCustomColorSet);
  SettingsPage::AddButtonRow(theme, Translations::CStr("Custom colors"), Translations::CStr(AppearanceSettingsLabels::DarkColors()), [settings]() {
    settings->BeginGroup(AppearanceSettings::kSettingsGroup);
    settings->SetBoolValue(AppearanceSettings::kUseCustomColorSet, true);
    for (const auto &role : AppearanceColors::Roles()) {
      settings->SetValue(role.key, role.dark_hex);
    }
    settings->Sync();
  });
  SettingsPage::AddButtonRow(theme, Translations::CStr("Custom colors"), Translations::CStr(AppearanceSettingsLabels::ResetColors()), [settings]() {
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
  SettingsPage::AddBoolRadios(tabbar, settings, AppearanceSettings::kTabBarSystemColor, AppearanceSettingsLabels::TabBarCustom(),
                             AppearanceSettingsLabels::TabBarSystem(), AppearanceSettings::kDefaultTabBarSystemColor);
  SettingsPage::AddToggle(tabbar, settings, AppearanceSettings::kTabBarGradient, AppearanceSettingsLabels::TabBarGradient(), nullptr,
                          AppearanceSettings::kDefaultTabBarGradient);
  SettingsPage::AddColorButton(tabbar, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kTabBarColor, "Tab bar color",
                              "#404040");

  AdwPreferencesGroup *playlist = SettingsPage::AddGroup(page, "Playlist");
  const std::string playlist_color = settings->Value(AppearanceSettings::kPlaylistPlayingSongColor, "#6696e3");
  SettingsPage::AddChoiceRadios(playlist, settings, nullptr, "Playlist playing song color",
                               {{"system", AppearanceSettingsLabels::PlaylistSystem()}, {"custom", AppearanceSettingsLabels::PlaylistCustom()}},
                               SettingsControls::PlaylistColorIsSystem(settings->Value(AppearanceSettings::kPlaylistPlayingSongColor))
                                   ? "system"
                                   : "custom",
                               [settings](const std::string &id) {
                                 settings->BeginGroup(AppearanceSettings::kSettingsGroup);
                                 const std::string current = settings->Value(AppearanceSettings::kPlaylistPlayingSongColor);
                                 settings->SetValue(AppearanceSettings::kPlaylistPlayingSongColor,
                                                    SettingsControls::PlaylistPlayingSongColor(id == "system", current));
                                 settings->Sync();
                               });
  SettingsPage::AddColorButton(playlist, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kPlaylistPlayingSongColor,
                              "Custom color", playlist_color.empty() ? "#6696e3" : playlist_color.c_str());

  AdwPreferencesGroup *background = SettingsPage::AddGroup(page, "Background");
  SettingsPage::AddChoiceRadios(background, settings, AppearanceSettings::kBackgroundImageType, "Background",
                               {{"0", AppearanceSettingsLabels::DefaultBackground()},
                                {"1", AppearanceSettingsLabels::NoBackground()},
                                {"2", "Custom image"},
                                {"3", AppearanceSettingsLabels::AlbumCover()},
                                {"4", "Strawberry"}},
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
                         {{"1", AppearanceSettingsLabels::UpperLeft()},
                          {"2", AppearanceSettingsLabels::UpperRight()},
                          {"3", AppearanceSettingsLabels::Middle()},
                          {"4", AppearanceSettingsLabels::BottomLeft()},
                          {"5", AppearanceSettingsLabels::BottomRight()}},
                         std::to_string(static_cast<int>(AppearanceSettings::kDefaultBackgroundImagePosition)));
  SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageStretch, AppearanceSettingsLabels::Stretch(), nullptr,
                          AppearanceSettings::kDefaultBackgroundImageStretch);
  SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageKeepAspectRatio, AppearanceSettingsLabels::KeepAspect(),
                          nullptr, AppearanceSettings::kDefaultBackgroundImageKeepAspectRatio);
  SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageDoNotCut, AppearanceSettingsLabels::DoNotCut(), nullptr,
                          AppearanceSettings::kDefaultBackgroundImageDoNotCut);
  SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageMaxSize, AppearanceSettingsLabels::MaxCoverSize(),
                            AppearanceSettings::kDefaultBackgroundImageMaxSize);
  const auto blur = SettingsControls::BackgroundBlur();
  SettingsPage::AddIntScale(background, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kBackgroundImageBlurRadius,
                           AppearanceSettingsLabels::BlurAmount(), AppearanceSettings::kDefaultBackgroundImageBlurRadius, static_cast<int>(blur.min),
                           static_cast<int>(blur.max), static_cast<int>(blur.step));
  const auto opacity = SettingsControls::BackgroundOpacity();
  SettingsPage::AddIntScale(background, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kBackgroundImageOpacityLevel,
                           "Opacity", AppearanceSettings::kDefaultBackgroundImageOpacityLevel, static_cast<int>(opacity.min),
                           static_cast<int>(opacity.max), static_cast<int>(opacity.step));

  AdwPreferencesGroup *icons = SettingsPage::AddGroup(page, "Icon sizes");
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeTabbarSmallMode, AppearanceSettingsLabels::TabbarSmall(),
                            AppearanceSettings::kDefaultIconSizeTabbarSmallMode);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeTabbarLargeMode, AppearanceSettingsLabels::TabbarLarge(),
                            AppearanceSettings::kDefaultIconSizeTabbarLargeMode);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizePlayControlButtons, AppearanceSettingsLabels::PlayControls(),
                            AppearanceSettings::kDefaultIconSizePlayControlButtons);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizePlaylistButtons, AppearanceSettingsLabels::PlaylistButtons(),
                            AppearanceSettings::kDefaultIconSizePlaylistButtons);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeLeftPanelButtons, AppearanceSettingsLabels::FilesPlaylistsQueue(),
                            AppearanceSettings::kDefaultIconSizeLeftPanelButtons);
  SettingsPage::AddIntEntry(icons, settings, AppearanceSettings::kIconSizeConfigureButtons, AppearanceSettingsLabels::ConfigureButtons(),
                            AppearanceSettings::kDefaultIconSizeConfigureButtons);
  return page;
}
