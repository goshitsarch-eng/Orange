#include "settings/playlistsettingspage.h"

#include "constants/playlistsettings.h"
#include "playlist/playlistsaveoptions.h"
#include "settings/playlistsettingscontrols.h"
#include "settings/playlistsettingslabels.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"

AdwPreferencesPage *PlaylistSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(PlaylistSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Playlist", "view-list-symbolic");
  AdwPreferencesGroup *look = SettingsPage::AddGroup(page, "Appearance");
  SettingsPage::AddToggle(look, settings, PlaylistSettings::kAlternatingRowColors, PlaylistSettingsLabels::Alternating(), nullptr,
                          PlaylistSettings::kDefaultAlternatingRowColors);
  GtkWidget *bars = SettingsPage::AddToggle(look, settings, PlaylistSettings::kShowBars, PlaylistSettingsLabels::Bars(), nullptr,
                                            PlaylistSettings::kDefaultShowBars);
  GtkWidget *glow = SettingsPage::AddToggle(look, settings, PlaylistSettings::kGlowEffect, PlaylistSettingsLabels::Glow(), nullptr,
                                            PlaylistSettingsControls::EffectiveGlow(settings->BoolValue(PlaylistSettings::kShowBars, PlaylistSettings::kDefaultShowBars),
                                                                                     settings->BoolValue(PlaylistSettings::kGlowEffect, PlaylistSettings::kDefaultGlowEffect)));
  gtk_widget_set_sensitive(glow, PlaylistSettingsControls::GlowToggleEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(bars))) ? TRUE : FALSE);
  g_object_set_data(G_OBJECT(bars), "glow-row", glow);
  g_object_set_data(G_OBJECT(bars), "settings", settings);
  g_signal_connect(bars, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     auto *glow_row = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "glow-row"));
                     auto *s = static_cast<Settings *>(g_object_get_data(G_OBJECT(row), "settings"));
                     const bool bars_on = adw_switch_row_get_active(row);
                     if (glow_row) {
                       gtk_widget_set_sensitive(glow_row, PlaylistSettingsControls::GlowToggleEnabled(bars_on) ? TRUE : FALSE);
                       if (!bars_on && ADW_IS_SWITCH_ROW(glow_row)) {
                         adw_switch_row_set_active(ADW_SWITCH_ROW(glow_row), FALSE);
                       }
                     }
                     if (s && !PlaylistSettingsControls::EffectiveGlow(bars_on, s->BoolValue(PlaylistSettings::kGlowEffect, false))) {
                       s->BeginGroup(PlaylistSettings::kSettingsGroup);
                       s->SetBoolValue(PlaylistSettings::kGlowEffect, false);
                       s->Sync();
                     }
                   }),
                   nullptr);
  SettingsPage::AddToggle(look, settings, PlaylistSettings::kShowToolbar, PlaylistSettingsLabels::Toolbar(), nullptr,
                          PlaylistSettings::kDefaultShowToolbar);

  AdwPreferencesGroup *behaviour = SettingsPage::AddGroup(page, "Behaviour");
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kWarnClosePlaylist, PlaylistSettingsLabels::WarnClose(), nullptr,
                          PlaylistSettings::kDefaultWarnClosePlaylist);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kContinueOnError, PlaylistSettingsLabels::ContinueOnError(), nullptr,
                          PlaylistSettings::kDefaultContinueOnError);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kGreyoutSongsStartup, PlaylistSettingsLabels::GreyoutStartup(), nullptr,
                          PlaylistSettings::kDefaultGreyoutSongsStartup);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kGreyoutSongsPlay, PlaylistSettingsLabels::GreyoutPlay(), nullptr,
                          PlaylistSettings::kDefaultGreyoutSongsPlay);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kSelectTrack, PlaylistSettingsLabels::SelectTrack(), nullptr,
                          PlaylistSettings::kDefaultSelectTrack);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kPlaylistClear, PlaylistSettingsLabels::Clear(), nullptr,
                          PlaylistSettings::kDefaultPlaylistClear);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kAutoSort, PlaylistSettingsLabels::AutoSort(), nullptr,
                          PlaylistSettings::kDefaultAutoSort);
  SettingsPage::AddIntCombo(behaviour, settings, PlaylistSettings::kSettingsGroup, PlaylistSettings::kPathType,
                            PlaylistSettingsLabels::PathsGroup(),
                            {{"0", PlaylistSaveOptions::DialogLabel(PlaylistSettings::PathType::Automatic)},
                             {"1", PlaylistSaveOptions::DialogLabel(PlaylistSettings::PathType::Absolute)},
                             {"2", PlaylistSaveOptions::DialogLabel(PlaylistSettings::PathType::Relative)},
                             {"3", PlaylistSaveOptions::AskWhenSaving()}},
                            static_cast<int>(PlaylistSettings::kDefaultPathType));

  AdwPreferencesGroup *meta = SettingsPage::AddGroup(page, "Metadata");
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kEditMetadataInline, PlaylistSettingsLabels::InlineEdit(), nullptr,
                          PlaylistSettings::kDefaultEditMetadataInline);
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kWriteMetadata, PlaylistSettingsLabels::WriteMetadata(), nullptr,
                          PlaylistSettings::kDefaultWriteMetadata);
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kDeleteFiles, PlaylistSettingsLabels::DeleteFiles(), nullptr,
                          PlaylistSettings::kDefaultDeleteFiles);
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kStretchColumns, "Stretch columns", nullptr,
                          PlaylistSettings::kDefaultStretchColumns);
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kRatingLocked, "Lock rating column", nullptr, PlaylistSettings::kDefaultRatingLocked);
  SettingsPage::AddButtonRow(meta, Translations::CStr("Visible columns"), Translations::CStr("Configure…"), [](GtkWidget *button) {
    GtkWindow *parent = nullptr;
    GtkRoot *root = button ? gtk_widget_get_root(button) : nullptr;
    if (GTK_IS_WINDOW(root)) {
      parent = GTK_WINDOW(root);
    }
    Dialogs::PlaylistColumns(parent, {});
  });
  return page;
}
