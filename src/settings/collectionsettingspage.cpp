#include "settings/collectionsettingspage.h"

#include "constants/collectionsettings.h"
#include "collection/collectionbackend.h"
#include "collection/collectioniconcache.h"
#include "collection/collectionstats.h"
#include "core/application.h"
#include "settings/collectionsettingslabels.h"
#include "settings/settingscontrols.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

#include <gio/gio.h>

#include <string>

namespace {

struct FolderListState {
  Application *app = nullptr;
  GtkWidget *list = nullptr;
  GtkWidget *status = nullptr;
};

GtkWindow *WindowForWidget(GtkWidget *widget) {
  for (GtkWidget *current = widget; current; current = gtk_widget_get_parent(current)) {
    if (GTK_IS_WINDOW(current)) {
      return GTK_WINDOW(current);
    }
  }
  GtkRoot *root = widget ? gtk_widget_get_root(widget) : nullptr;
  return GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
}

void RefreshFolderList(FolderListState *state) {
  if (!state || !state->list || !state->app) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(state->list)) {
    gtk_list_box_remove(GTK_LIST_BOX(state->list), child);
  }
  const std::vector<CollectionDirectory> directories = state->app->collection()->backend()->Directories();
  if (state->status) {
    gtk_label_set_text(GTK_LABEL(state->status),
                       directories.empty() ? Translations::CStr("No collection folders")
                                           : (std::to_string(directories.size()) + " " + Translations::Tr("folder(s)")).c_str());
  }
  for (const CollectionDirectory &directory : directories) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), directory.path.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), directory.subdirs ? Translations::CStr("Including subfolders")
                                                                      : Translations::CStr("This folder only"));
    GtkWidget *rescan = gtk_button_new_with_label(Translations::CStr("Rescan"));
    GtkWidget *remove = gtk_button_new_with_label(Translations::CStr(CollectionSettingsLabels::RemoveFolder()));
    gtk_widget_add_css_class(remove, "destructive-action");
    g_object_set_data(G_OBJECT(rescan), "folder-state", state);
    g_object_set_data(G_OBJECT(remove), "folder-state", state);
    g_object_set_data(G_OBJECT(rescan), "directory-id", GINT_TO_POINTER(directory.id));
    g_object_set_data(G_OBJECT(remove), "directory-id", GINT_TO_POINTER(directory.id));
    g_signal_connect(rescan, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<FolderListState *>(g_object_get_data(G_OBJECT(button), "folder-state"));
                       const int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "directory-id"));
                       if (self && self->app) {
                         self->app->collection()->RescanDirectory(id);
                       }
                     })),
                     nullptr);
    g_signal_connect(remove, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<FolderListState *>(g_object_get_data(G_OBJECT(button), "folder-state"));
                       const int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "directory-id"));
                       if (self && self->app) {
                         self->app->collection()->RemoveDirectory(id);
                         RefreshFolderList(self);
                       }
                     })),
                     nullptr);
    adw_action_row_add_suffix(row, rescan);
    adw_action_row_add_suffix(row, remove);
    gtk_list_box_append(GTK_LIST_BOX(state->list), GTK_WIDGET(row));
  }
}

}  // namespace

AdwPreferencesPage *CollectionSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(CollectionSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Collection", "media-optical-cd-audio-symbolic");
  SettingsPage::AddDescription(SettingsPage::AddGroup(page), CollectionSettingsLabels::Intro());
  AdwPreferencesGroup *scan = SettingsPage::AddGroup(page, CollectionSettingsLabels::AutomaticUpdating());
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kStartupScan, CollectionSettingsLabels::StartupScan(), nullptr,
                          CollectionSettings::kDefaultStartupScan);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kMonitor, CollectionSettingsLabels::Monitor(), nullptr, CollectionSettings::kDefaultMonitor);
  GtkWidget *song_tracking = SettingsPage::AddToggle(scan, settings, CollectionSettings::kSongTracking, CollectionSettingsLabels::SongTracking(),
                                                     nullptr, CollectionSettings::kDefaultSongTracking);
  GtkWidget *mark_unavailable = SettingsPage::AddToggle(scan, settings, CollectionSettings::kMarkSongsUnavailable,
                                                        CollectionSettingsLabels::MarkUnavailable(), nullptr,
                                                        CollectionSettings::kDefaultMarkSongsUnavailable);
  const bool tracking_on = adw_switch_row_get_active(ADW_SWITCH_ROW(song_tracking));
  gtk_widget_set_sensitive(mark_unavailable, CollectionSongTracking::MarkUnavailableEnabled(tracking_on));
  if (tracking_on) {
    adw_switch_row_set_active(ADW_SWITCH_ROW(mark_unavailable), TRUE);
  }
  g_object_set_data(G_OBJECT(song_tracking), "mark-unavailable", mark_unavailable);
  g_signal_connect(song_tracking, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     auto *mark = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "mark-unavailable"));
                     if (!mark) {
                       return;
                     }
                     const bool tracking = adw_switch_row_get_active(row);
                     gtk_widget_set_sensitive(mark, CollectionSongTracking::MarkUnavailableEnabled(tracking));
                     if (tracking) {
                       adw_switch_row_set_active(ADW_SWITCH_ROW(mark), TRUE);
                     }
                   }),
                   nullptr);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kSongENUR128LoudnessAnalysis, CollectionSettingsLabels::EbuAnalysis(), nullptr,
                          CollectionSettings::kDefaultSongENUR128LoudnessAnalysis);
  SettingsPage::AddIntEntry(scan, settings, CollectionSettings::kExpireUnavailableSongs, CollectionSettingsLabels::ExpireUnavailable(),
                            CollectionSettings::kDefaultExpireUnavailableSongs);
  SettingsPage::AddDescription(scan, CollectionSettingsLabels::Days());
  SettingsPage::AddEntry(scan, settings, CollectionSettings::kCoverArtPatterns, CollectionSettingsLabels::CoverPatterns(),
                         "cover.jpg,folder.jpg,front.jpg,album.jpg");
  SettingsPage::AddDescription(scan, CollectionSettingsLabels::CoverPatternsHint());

  AdwPreferencesGroup *display = SettingsPage::AddGroup(page, CollectionSettingsLabels::DisplayOptions());
  SettingsPage::AddToggle(display, settings, CollectionSettings::kAutoOpen, CollectionSettingsLabels::AutoOpen(), nullptr,
                          CollectionSettings::kDefaultAutoOpen);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kShowDividers, CollectionSettingsLabels::ShowDividers(), nullptr,
                          CollectionSettings::kDefaultShowDividers);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kPrettyCovers, CollectionSettingsLabels::PrettyCovers(), nullptr,
                          CollectionSettings::kDefaultPrettyCovers);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kVariousArtists, CollectionSettingsLabels::VariousArtists(), nullptr,
                          CollectionSettings::kDefaultVariousArtists);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kSkipArticlesForArtists, CollectionSettingsLabels::SkipArtistArticles(), nullptr,
                          CollectionSettings::kDefaultSkipArticlesForArtists);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kSkipArticlesForAlbums, CollectionSettingsLabels::SkipAlbumArticles(), nullptr,
                          CollectionSettings::kDefaultSkipArticlesForAlbums);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kUseSortTags, CollectionSettingsLabels::UseSortTags(), nullptr,
                          CollectionSettings::kDefaultUseSortTags);

  AdwPreferencesGroup *cache = SettingsPage::AddGroup(page, CollectionSettingsLabels::CacheGroup());
  GtkWidget *icon_size =
      SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsCacheSize, CollectionSettingsLabels::CacheSize(), CollectionSettings::kSettingsCacheSizeDefault);
  const std::vector<std::pair<std::string, std::string>> units = {{"0", "KB"}, {"1", "MB"}, {"2", "GB"}, {"3", "TB"}};
  GtkWidget *icon_unit =
      SettingsPage::AddIntCombo(cache, settings, CollectionSettings::kSettingsGroup, CollectionSettings::kSettingsCacheSizeUnit, "Icon cache unit",
                                units, static_cast<int>(CollectionSettings::kDefaultSettingsCacheSizeUnit));
  GtkWidget *disk_enable = SettingsPage::AddToggle(cache, settings, CollectionSettings::kSettingsDiskCacheEnable,
                                                   CollectionSettingsLabels::EnableDiskCache(), nullptr,
                                                   CollectionSettings::kDefaultSettingsDiskCacheEnable);
  GtkWidget *disk_size = SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsDiskCacheSize,
                                                   CollectionSettingsLabels::DiskCacheSize(), CollectionSettings::kSettingsDiskCacheSizeDefault);
  GtkWidget *disk_unit = SettingsPage::AddIntCombo(cache, settings, CollectionSettings::kSettingsGroup, CollectionSettings::kSettingsDiskCacheSizeUnit,
                                                   "Disk cache unit", units, static_cast<int>(CollectionSettings::kDefaultSettingsDiskCacheSizeUnit));
  AdwActionRow *in_use = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_use), Translations::CStr(CollectionStats::CacheInUseTitle()));
  adw_action_row_set_subtitle(in_use, CollectionIconCache::InUseLabel().c_str());
  GtkWidget *clear_cache = gtk_button_new_with_label(Translations::CStr(CollectionStats::ClearCacheLabel()));
  gtk_widget_add_css_class(clear_cache, "destructive-action");
  g_object_set_data(G_OBJECT(clear_cache), "in-use-row", in_use);
  g_signal_connect(clear_cache, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     CollectionIconCache::Clear();
                     if (auto *row = ADW_ACTION_ROW(g_object_get_data(G_OBJECT(button), "in-use-row"))) {
                       adw_action_row_set_subtitle(row, CollectionIconCache::InUseLabel().c_str());
                     }
                   }),
                   nullptr);
  adw_action_row_add_suffix(in_use, clear_cache);
  adw_preferences_group_add(cache, GTK_WIDGET(in_use));
  auto clamp_cache_entry = [](GtkWidget *combo, GtkWidget *entry, bool disk) {
    if (!GTK_IS_EDITABLE(entry) || !ADW_IS_COMBO_ROW(combo)) {
      return;
    }
    const int unit = static_cast<int>(adw_combo_row_get_selected(ADW_COMBO_ROW(combo)));
    const int max = disk ? SettingsControls::DiskCacheSizeMax(unit) : SettingsControls::IconCacheSizeMax(unit);
    const int value = static_cast<int>(g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(entry)), nullptr, 10));
    const int clamped = SettingsControls::ClampCacheSize(value, max);
    if (clamped != value) {
      gtk_editable_set_text(GTK_EDITABLE(entry), std::to_string(clamped).c_str());
    }
  };
  g_object_set_data(G_OBJECT(icon_unit), "size-entry", icon_size);
  g_signal_connect(icon_unit, "notify::selected", G_CALLBACK(+[](AdwComboRow *combo, GParamSpec *, gpointer) {
                     auto *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "size-entry"));
                     if (!GTK_IS_EDITABLE(entry)) {
                       return;
                     }
                     const int unit = static_cast<int>(adw_combo_row_get_selected(combo));
                     const int max = SettingsControls::IconCacheSizeMax(unit);
                     const int value = static_cast<int>(g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(entry)), nullptr, 10));
                     const int clamped = SettingsControls::ClampCacheSize(value, max);
                     if (clamped != value) {
                       gtk_editable_set_text(GTK_EDITABLE(entry), std::to_string(clamped).c_str());
                     }
                   }),
                   nullptr);
  g_object_set_data(G_OBJECT(disk_unit), "size-entry", disk_size);
  g_signal_connect(disk_unit, "notify::selected", G_CALLBACK(+[](AdwComboRow *combo, GParamSpec *, gpointer) {
                     auto *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "size-entry"));
                     if (!GTK_IS_EDITABLE(entry)) {
                       return;
                     }
                     const int unit = static_cast<int>(adw_combo_row_get_selected(combo));
                     const int max = SettingsControls::DiskCacheSizeMax(unit);
                     const int value = static_cast<int>(g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(entry)), nullptr, 10));
                     const int clamped = SettingsControls::ClampCacheSize(value, max);
                     if (clamped != value) {
                       gtk_editable_set_text(GTK_EDITABLE(entry), std::to_string(clamped).c_str());
                     }
                   }),
                   nullptr);
  clamp_cache_entry(icon_unit, icon_size, false);
  clamp_cache_entry(disk_unit, disk_size, true);
  const bool disk_on = settings->BoolValue(CollectionSettings::kSettingsDiskCacheEnable, CollectionSettings::kDefaultSettingsDiskCacheEnable);
  gtk_widget_set_sensitive(disk_size, disk_on);
  gtk_widget_set_sensitive(disk_unit, disk_on);
  gtk_widget_set_sensitive(GTK_WIDGET(in_use), disk_on);
  g_object_set_data(G_OBJECT(disk_enable), "disk-size", disk_size);
  g_object_set_data(G_OBJECT(disk_enable), "disk-unit", disk_unit);
  g_object_set_data(G_OBJECT(disk_enable), "in-use", in_use);
  g_signal_connect(disk_enable, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     const bool enabled = adw_switch_row_get_active(row);
                     if (auto *size = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "disk-size"))) {
                       gtk_widget_set_sensitive(size, enabled);
                     }
                     if (auto *unit = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "disk-unit"))) {
                       gtk_widget_set_sensitive(unit, enabled);
                     }
                     if (auto *use = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "in-use"))) {
                       gtk_widget_set_sensitive(use, enabled);
                     }
                   }),
                   nullptr);

  AdwPreferencesGroup *tags = SettingsPage::AddGroup(page, CollectionSettingsLabels::PlaycountsGroup());
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kSavePlayCounts, "Save playcounts to song tags when possible", nullptr,
                          CollectionSettings::kDefaultSavePlayCounts);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kSaveRatings, "Save ratings to song tags when possible", nullptr,
                          CollectionSettings::kDefaultSaveRatings);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kOverwritePlaycount, "Overwrite database playcount when songs are re-read from disk",
                          nullptr, CollectionSettings::kDefaultOverwritePlaycount);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kOverwriteRating, "Overwrite database rating when songs are re-read from disk", nullptr,
                          CollectionSettings::kDefaultOverwriteRating);
  if (app) {
    SettingsPage::AddButtonRow(tags, "", CollectionStats::SaveNowLabel(), [app](GtkWidget *button) {
      AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(CollectionStats::ConfirmTitle()),
                                                                    Translations::CStr(CollectionStats::ConfirmText())));
      adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "write", Translations::CStr("Write"), nullptr);
      adw_alert_dialog_set_response_appearance(dialog, "write", ADW_RESPONSE_SUGGESTED);
      g_object_set_data(G_OBJECT(dialog), "app", app);
      g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *response, gpointer data) {
                         if (g_strcmp0(response, "write") != 0) {
                           return;
                         }
                         if (auto *application = static_cast<Application *>(data)) {
                           application->collection()->SyncPlaycountAndRatingToFilesAsync();
                         }
                       }),
                       app);
      adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(WindowForWidget(button)));
    });
  }
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kDeleteFiles, CollectionSettingsLabels::DeleteFiles(), nullptr,
                          CollectionSettings::kDefaultDeleteFiles);

  AdwPreferencesGroup *dirs = SettingsPage::AddGroup(page, "Folders");
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  GtkWidget *status = gtk_label_new("");
  gtk_widget_add_css_class(status, "dim-label");
  gtk_label_set_xalign(GTK_LABEL(status), 0);
  auto *folder_state = new FolderListState();
  folder_state->app = app;
  folder_state->list = list;
  folder_state->status = status;
  g_object_set_data_full(G_OBJECT(page), "folder-state", folder_state, [](gpointer p) { delete static_cast<FolderListState *>(p); });
  if (app) {
    RefreshFolderList(folder_state);
    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *add = gtk_button_new_with_label(Translations::CStr(CollectionSettingsLabels::AddFolder()));
    GtkWidget *rescan_all = gtk_button_new_with_label(Translations::CStr("Rescan all"));
    GtkWidget *full_scan = gtk_button_new_with_label(Translations::CStr("Full rescan"));
    gtk_widget_add_css_class(add, "suggested-action");
    gtk_box_append(GTK_BOX(buttons), add);
    gtk_box_append(GTK_BOX(buttons), rescan_all);
    gtk_box_append(GTK_BOX(buttons), full_scan);
    g_signal_connect(add, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                       auto *self = static_cast<FolderListState *>(data);
                       GtkFileDialog *chooser = gtk_file_dialog_new();
                       gtk_file_dialog_set_title(chooser, Translations::CStr("Add collection folder"));
                       gtk_file_dialog_select_folder(chooser, WindowForWidget(GTK_WIDGET(button)), nullptr,
                                                     +[](GObject *source, GAsyncResult *result, gpointer user) {
                                                       auto *state = static_cast<FolderListState *>(user);
                                                       GError *error = nullptr;
                                                       GFile *file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
                                                       if (!file) {
                                                         if (error) {
                                                           g_error_free(error);
                                                         }
                                                         return;
                                                       }
                                                       gchar *path = g_file_get_path(file);
                                                       if (path && state && state->app) {
                                                         state->app->collection()->AddDirectory(path, true);
                                                         RefreshFolderList(state);
                                                       }
                                                       g_free(path);
                                                       g_object_unref(file);
                                                     },
                                                     self);
                     })),
                     folder_state);
    g_signal_connect(rescan_all, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *self = static_cast<FolderListState *>(data);
                       if (self && self->app) {
                         self->app->collection()->IncrementalScan();
                       }
                     })),
                     folder_state);
    g_signal_connect(full_scan, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *self = static_cast<FolderListState *>(data);
                       if (self && self->app) {
                         self->app->collection()->FullScan();
                       }
                     })),
                     folder_state);
    adw_preferences_group_add(dirs, buttons);
  }
  adw_preferences_group_add(dirs, list);
  adw_preferences_group_add(dirs, status);
  return page;
}
