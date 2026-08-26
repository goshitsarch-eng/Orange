#include "settings/playlistsettingspage.h"

#include "constants/playlistsettings.h"
#include "playlist/playlistsaveoptions.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "ui/dialogs.h"

AdwPreferencesPage *PlaylistSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(PlaylistSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Playlist", "view-list-symbolic");
  AdwPreferencesGroup *look = SettingsPage::AddGroup(page, "Appearance");
  SettingsPage::AddToggle(look, settings, PlaylistSettings::kAlternatingRowColors, "Alternating row colors", nullptr,
                          PlaylistSettings::kDefaultAlternatingRowColors);
  SettingsPage::AddToggle(look, settings, PlaylistSettings::kShowBars, "Show bars on the current track", nullptr, PlaylistSettings::kDefaultShowBars);
  SettingsPage::AddToggle(look, settings, PlaylistSettings::kGlowEffect, "Glow effect on the current track", nullptr, PlaylistSettings::kDefaultGlowEffect);
  SettingsPage::AddToggle(look, settings, PlaylistSettings::kShowToolbar, "Show playlist toolbar", nullptr, PlaylistSettings::kDefaultShowToolbar);

  AdwPreferencesGroup *behaviour = SettingsPage::AddGroup(page, "Behaviour");
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kWarnClosePlaylist, "Warn when closing a playlist", nullptr,
                          PlaylistSettings::kDefaultWarnClosePlaylist);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kContinueOnError, "Continue on error", nullptr,
                          PlaylistSettings::kDefaultContinueOnError);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kGreyoutSongsStartup, "Grey out unavailable songs at startup", nullptr,
                          PlaylistSettings::kDefaultGreyoutSongsStartup);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kGreyoutSongsPlay, "Grey out unavailable songs while playing", nullptr,
                          PlaylistSettings::kDefaultGreyoutSongsPlay);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kSelectTrack, "Select the current track", nullptr, PlaylistSettings::kDefaultSelectTrack);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kPlaylistClear, "Allow clearing the playlist", nullptr,
                          PlaylistSettings::kDefaultPlaylistClear);
  SettingsPage::AddToggle(behaviour, settings, PlaylistSettings::kAutoSort, "Auto-sort the playlist", nullptr, PlaylistSettings::kDefaultAutoSort);
  SettingsPage::AddIntCombo(behaviour, settings, PlaylistSettings::kSettingsGroup, PlaylistSettings::kPathType,
                            PlaylistSaveOptions::SettingsPrompt(),
                            {{"0", PlaylistSaveOptions::DialogLabel(PlaylistSettings::PathType::Automatic)},
                             {"1", PlaylistSaveOptions::DialogLabel(PlaylistSettings::PathType::Absolute)},
                             {"2", PlaylistSaveOptions::DialogLabel(PlaylistSettings::PathType::Relative)},
                             {"3", PlaylistSaveOptions::AskWhenSaving()}},
                            static_cast<int>(PlaylistSettings::kDefaultPathType));

  AdwPreferencesGroup *meta = SettingsPage::AddGroup(page, "Metadata");
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kEditMetadataInline, "Edit metadata inline", nullptr,
                          PlaylistSettings::kDefaultEditMetadataInline);
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kWriteMetadata, "Write metadata to playlist files", nullptr,
                          PlaylistSettings::kDefaultWriteMetadata);
  SettingsPage::AddToggle(meta, settings, PlaylistSettings::kDeleteFiles, "Allow deleting files from the playlist", nullptr,
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
