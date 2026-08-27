#include "settings/appearancesettingspage.h"

#include "constants/appearancesettings.h"
#include "core/appearancecolors.h"
#include "core/appearancestyle.h"
#include "settings/appearancesettingslabels.h"
#include "settings/settingscontrols.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

#include <gio/gio.h>

#include <string>

namespace {

struct AppearanceEnableState {
  Settings *settings = nullptr;
  GtkWidget *dark_mode = nullptr;
  GtkWidget *colors = nullptr;
  GtkWidget *dark_colors = nullptr;
  GtkWidget *reset_colors = nullptr;
  GtkWidget *tabbar_color = nullptr;
  GtkWidget *playlist_color = nullptr;
  GtkWidget *filename = nullptr;
  GtkWidget *choose = nullptr;
  GtkWidget *position = nullptr;
  GtkWidget *stretch = nullptr;
  GtkWidget *keep = nullptr;
  GtkWidget *cut = nullptr;
  GtkWidget *max_size = nullptr;
  GtkWidget *blur = nullptr;
  GtkWidget *opacity = nullptr;
};

void SetSensitive(GtkWidget *widget, bool enabled) {
  if (widget) {
    gtk_widget_set_sensitive(widget, enabled ? TRUE : FALSE);
  }
}

void ApplyAppearanceEnable(AppearanceEnableState *state) {
  if (!state || !state->settings) {
    return;
  }
  state->settings->BeginGroup(AppearanceSettings::kSettingsGroup);
  const std::string style = state->settings->Value(AppearanceSettings::kStyle, "");
  const bool use_custom =
      state->settings->BoolValue(AppearanceSettings::kUseCustomColorSet, AppearanceSettings::kDefaultUseCustomColorSet);
  const bool tabbar_system =
      state->settings->BoolValue(AppearanceSettings::kTabBarSystemColor, AppearanceSettings::kDefaultTabBarSystemColor);
  const bool playlist_system =
      SettingsControls::PlaylistColorIsSystem(state->settings->Value(AppearanceSettings::kPlaylistPlayingSongColor));
  const auto type = AppearanceEnable::TypeFromId(state->settings->Value(
      AppearanceSettings::kBackgroundImageType, std::to_string(static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType))));
  const bool stretch = state->stretch ? adw_switch_row_get_active(ADW_SWITCH_ROW(state->stretch))
                                      : state->settings->BoolValue(AppearanceSettings::kBackgroundImageStretch,
                                                                   AppearanceSettings::kDefaultBackgroundImageStretch);
  const bool keep = state->keep ? adw_switch_row_get_active(ADW_SWITCH_ROW(state->keep))
                                : state->settings->BoolValue(AppearanceSettings::kBackgroundImageKeepAspectRatio,
                                                             AppearanceSettings::kDefaultBackgroundImageKeepAspectRatio);
  const bool palette = AppearanceStyle::HasCustomPalette(style);
  const bool options = AppearanceEnable::BackgroundOptionsEnabled(type);
  SetSensitive(state->dark_mode, AppearanceStyle::HasDarkMode(style));
  const bool colors_on = palette && AppearanceEnable::CustomColorsEnabled(use_custom);
  SetSensitive(state->colors, colors_on);
  SetSensitive(state->dark_colors, colors_on);
  SetSensitive(state->reset_colors, colors_on);
  SetSensitive(state->tabbar_color, AppearanceEnable::TabBarColorEnabled(tabbar_system));
  SetSensitive(state->playlist_color, AppearanceEnable::PlaylistCustomColorEnabled(playlist_system));
  SetSensitive(state->filename, AppearanceEnable::BackgroundFilenameEnabled(type));
  SetSensitive(state->choose, AppearanceEnable::BackgroundFilenameEnabled(type));
  SetSensitive(state->position, options);
  SetSensitive(state->stretch, options);
  SetSensitive(state->keep, options && AppearanceEnable::KeepAspectEnabled(stretch));
  SetSensitive(state->cut, options && AppearanceEnable::DoNotCutEnabled(stretch, keep));
  SetSensitive(state->max_size, options && AppearanceEnable::MaxSizeEnabled(stretch));
  SetSensitive(state->blur, options);
  SetSensitive(state->opacity, options);
}

}  // namespace

AdwPreferencesPage *AppearanceSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(AppearanceSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Appearance", "applications-graphics-symbolic");
  auto *enable = new AppearanceEnableState();
  enable->settings = settings;
  g_object_set_data_full(G_OBJECT(page), "appearance-enable", enable, [](gpointer p) { delete static_cast<AppearanceEnableState *>(p); });

  AdwPreferencesGroup *theme = SettingsPage::AddGroup(page, "Theme");
  enable->dark_mode = SettingsPage::AddToggle(theme, settings, AppearanceSettings::kDarkMode, AppearanceSettingsLabels::DarkMode(),
                                              AppearanceSettingsLabels::DarkRestart(), AppearanceSettings::kDefaultDarkMode);
  SettingsPage::AddToggle(theme, settings, AppearanceSettings::kSystemThemeIcons, AppearanceSettingsLabels::SystemIcons(), nullptr,
                          AppearanceSettings::kDefaultSystemIcons);
  SettingsPage::AddCombo(theme, settings, AppearanceSettings::kStyle, "Style", AppearanceStyle::Choices(), "",
                         [enable](const std::string &) { ApplyAppearanceEnable(enable); });
  SettingsPage::AddDescription(theme, AppearanceSettingsLabels::StyleRestart());
  SettingsPage::AddBoolRadios(theme, settings, AppearanceSettings::kUseCustomColorSet, AppearanceSettingsLabels::SystemColorSet(),
                             AppearanceSettingsLabels::CustomColorSet(), AppearanceSettings::kDefaultUseCustomColorSet,
                             [enable](bool) { ApplyAppearanceEnable(enable); });
  enable->dark_colors =
      SettingsPage::AddButtonRow(theme, Translations::CStr("Custom colors"), Translations::CStr(AppearanceSettingsLabels::DarkColors()), [settings]() {
        settings->BeginGroup(AppearanceSettings::kSettingsGroup);
        settings->SetBoolValue(AppearanceSettings::kUseCustomColorSet, true);
        for (const auto &role : AppearanceColors::Roles()) {
          settings->SetValue(role.key, role.dark_hex);
        }
        settings->Sync();
      });
  enable->reset_colors =
      SettingsPage::AddButtonRow(theme, Translations::CStr("Custom colors"), Translations::CStr(AppearanceSettingsLabels::ResetColors()), [settings]() {
        settings->BeginGroup(AppearanceSettings::kSettingsGroup);
        settings->SetBoolValue(AppearanceSettings::kUseCustomColorSet, false);
        for (const auto &role : AppearanceColors::Roles()) {
          settings->Remove(role.key);
        }
        settings->Sync();
      });

  AdwPreferencesGroup *colors = SettingsPage::AddGroup(page, "Custom colors");
  enable->colors = GTK_WIDGET(colors);
  for (const auto &role : AppearanceColors::Roles()) {
    SettingsPage::AddColorButton(colors, settings, AppearanceSettings::kSettingsGroup, role.key, role.title, role.dark_hex);
  }

  AdwPreferencesGroup *tabbar = SettingsPage::AddGroup(page, "Tab bar");
  SettingsPage::AddBoolRadios(tabbar, settings, AppearanceSettings::kTabBarSystemColor, AppearanceSettingsLabels::TabBarCustom(),
                             AppearanceSettingsLabels::TabBarSystem(), AppearanceSettings::kDefaultTabBarSystemColor,
                             [enable](bool) { ApplyAppearanceEnable(enable); });
  SettingsPage::AddToggle(tabbar, settings, AppearanceSettings::kTabBarGradient, AppearanceSettingsLabels::TabBarGradient(), nullptr,
                          AppearanceSettings::kDefaultTabBarGradient);
  enable->tabbar_color = SettingsPage::AddColorButton(tabbar, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kTabBarColor,
                                                     "Tab bar color", "#404040");

  AdwPreferencesGroup *playlist = SettingsPage::AddGroup(page, "Playlist");
  const std::string playlist_color = settings->Value(AppearanceSettings::kPlaylistPlayingSongColor, "#6696e3");
  SettingsPage::AddChoiceRadios(playlist, settings, nullptr, "Playlist playing song color",
                               {{"system", AppearanceSettingsLabels::PlaylistSystem()}, {"custom", AppearanceSettingsLabels::PlaylistCustom()}},
                               SettingsControls::PlaylistColorIsSystem(settings->Value(AppearanceSettings::kPlaylistPlayingSongColor))
                                   ? "system"
                                   : "custom",
                               [settings, enable](const std::string &id) {
                                 settings->BeginGroup(AppearanceSettings::kSettingsGroup);
                                 const std::string current = settings->Value(AppearanceSettings::kPlaylistPlayingSongColor);
                                 settings->SetValue(AppearanceSettings::kPlaylistPlayingSongColor,
                                                    SettingsControls::PlaylistPlayingSongColor(id == "system", current));
                                 settings->Sync();
                                 ApplyAppearanceEnable(enable);
                               });
  enable->playlist_color = SettingsPage::AddColorButton(playlist, settings, AppearanceSettings::kSettingsGroup,
                                                       AppearanceSettings::kPlaylistPlayingSongColor, "Custom color",
                                                       playlist_color.empty() ? "#6696e3" : playlist_color.c_str());

  AdwPreferencesGroup *background = SettingsPage::AddGroup(page, "Background");
  SettingsPage::AddChoiceRadios(background, settings, AppearanceSettings::kBackgroundImageType, "Background",
                               {{"0", AppearanceSettingsLabels::DefaultBackground()},
                                {"1", AppearanceSettingsLabels::NoBackground()},
                                {"2", "Custom image"},
                                {"3", AppearanceSettingsLabels::AlbumCover()},
                                {"4", "Strawberry"}},
                               std::to_string(static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType)),
                               [enable](const std::string &) { ApplyAppearanceEnable(enable); });
  enable->filename = SettingsPage::AddEntry(background, settings, AppearanceSettings::kBackgroundImageFilename, "Custom background file");
  enable->choose = SettingsPage::AddButtonRow(background, "Custom background file", "Choose image…", [settings]() {
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
  enable->position = SettingsPage::AddCombo(background, settings, AppearanceSettings::kBackgroundImagePosition, "Position",
                                            {{"1", AppearanceSettingsLabels::UpperLeft()},
                                             {"2", AppearanceSettingsLabels::UpperRight()},
                                             {"3", AppearanceSettingsLabels::Middle()},
                                             {"4", AppearanceSettingsLabels::BottomLeft()},
                                             {"5", AppearanceSettingsLabels::BottomRight()}},
                                            std::to_string(static_cast<int>(AppearanceSettings::kDefaultBackgroundImagePosition)));
  enable->stretch = SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageStretch, AppearanceSettingsLabels::Stretch(),
                                            nullptr, AppearanceSettings::kDefaultBackgroundImageStretch);
  enable->keep = SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageKeepAspectRatio,
                                         AppearanceSettingsLabels::KeepAspect(), nullptr,
                                         AppearanceSettings::kDefaultBackgroundImageKeepAspectRatio);
  enable->cut = SettingsPage::AddToggle(background, settings, AppearanceSettings::kBackgroundImageDoNotCut, AppearanceSettingsLabels::DoNotCut(),
                                        nullptr, AppearanceSettings::kDefaultBackgroundImageDoNotCut);
  enable->max_size = SettingsPage::AddIntEntry(background, settings, AppearanceSettings::kBackgroundImageMaxSize,
                                               AppearanceSettingsLabels::MaxCoverSize(), AppearanceSettings::kDefaultBackgroundImageMaxSize);
  g_object_set_data(G_OBJECT(enable->stretch), "appearance-enable", enable);
  g_signal_connect(enable->stretch, "notify::active", G_CALLBACK(+[](AdwSwitchRow *, GParamSpec *, gpointer data) {
                     ApplyAppearanceEnable(static_cast<AppearanceEnableState *>(data));
                   }),
                   enable);
  g_object_set_data(G_OBJECT(enable->keep), "appearance-enable", enable);
  g_signal_connect(enable->keep, "notify::active", G_CALLBACK(+[](AdwSwitchRow *, GParamSpec *, gpointer data) {
                     ApplyAppearanceEnable(static_cast<AppearanceEnableState *>(data));
                   }),
                   enable);
  const auto blur = SettingsControls::BackgroundBlur();
  enable->blur = SettingsPage::AddIntScale(background, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kBackgroundImageBlurRadius,
                                           AppearanceSettingsLabels::BlurAmount(), AppearanceSettings::kDefaultBackgroundImageBlurRadius,
                                           static_cast<int>(blur.min), static_cast<int>(blur.max), static_cast<int>(blur.step));
  const auto opacity = SettingsControls::BackgroundOpacity();
  enable->opacity = SettingsPage::AddIntScale(background, settings, AppearanceSettings::kSettingsGroup, AppearanceSettings::kBackgroundImageOpacityLevel,
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
  ApplyAppearanceEnable(enable);
  return page;
}
