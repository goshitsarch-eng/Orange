#include "settings/collectionsettingspage.h"

#include "constants/collectionsettings.h"
#include "collection/collectionbackend.h"
#include "core/application.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

#include <gio/gio.h>

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
    GtkWidget *remove = gtk_button_new_with_label(Translations::CStr("Remove"));
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
  AdwPreferencesGroup *scan = SettingsPage::AddGroup(page, "Scanning");
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kStartupScan, "Scan collection on startup", nullptr,
                          CollectionSettings::kDefaultStartupScan);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kMonitor, "Watch folders for changes", nullptr, CollectionSettings::kDefaultMonitor);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kSongTracking, "Track songs with Chromaprint / AcoustID", nullptr,
                          CollectionSettings::kDefaultSongTracking);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kMarkSongsUnavailable, "Mark missing songs as unavailable", nullptr,
                          CollectionSettings::kDefaultMarkSongsUnavailable);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kSongENUR128LoudnessAnalysis, "EBU R128 loudness analysis", nullptr,
                          CollectionSettings::kDefaultSongENUR128LoudnessAnalysis);
  SettingsPage::AddIntEntry(scan, settings, CollectionSettings::kExpireUnavailableSongs, "Expire unavailable songs (days)",
                            CollectionSettings::kDefaultExpireUnavailableSongs);
  SettingsPage::AddEntry(scan, settings, CollectionSettings::kCoverArtPatterns, "Cover art filename patterns", "cover.jpg,folder.jpg,front.jpg,album.jpg");

  AdwPreferencesGroup *display = SettingsPage::AddGroup(page, "Display");
  SettingsPage::AddToggle(display, settings, CollectionSettings::kAutoOpen, "Auto-open collection items", nullptr, CollectionSettings::kDefaultAutoOpen);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kShowDividers, "Show artist / album dividers", nullptr,
                          CollectionSettings::kDefaultShowDividers);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kPrettyCovers, "Use pretty covers", nullptr, CollectionSettings::kDefaultPrettyCovers);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kVariousArtists, "Group various artists albums", nullptr,
                          CollectionSettings::kDefaultVariousArtists);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kSkipArticlesForArtists, "Skip “The / A / An” when sorting artists", nullptr,
                          CollectionSettings::kDefaultSkipArticlesForArtists);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kSkipArticlesForAlbums, "Skip “The / A / An” when sorting albums", nullptr,
                          CollectionSettings::kDefaultSkipArticlesForAlbums);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kUseSortTags, "Use sort tags", nullptr, CollectionSettings::kDefaultUseSortTags);

  AdwPreferencesGroup *cache = SettingsPage::AddGroup(page, "Cache");
  SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsCacheSize, "Icon cache size", CollectionSettings::kSettingsCacheSizeDefault);
  SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsCacheSizeUnit, "Icon cache unit (0 KB / 1 MB / 2 GB / 3 TB)",
                            static_cast<int>(CollectionSettings::kDefaultSettingsCacheSizeUnit));
  SettingsPage::AddToggle(cache, settings, CollectionSettings::kSettingsDiskCacheEnable, "Enable disk cache", nullptr,
                          CollectionSettings::kDefaultSettingsDiskCacheEnable);
  SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsDiskCacheSize, "Disk cache size",
                            CollectionSettings::kSettingsDiskCacheSizeDefault);

  AdwPreferencesGroup *tags = SettingsPage::AddGroup(page, "Tags");
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kSavePlayCounts, "Save playcounts to files", nullptr,
                          CollectionSettings::kDefaultSavePlayCounts);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kSaveRatings, "Save ratings to files", nullptr, CollectionSettings::kDefaultSaveRatings);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kOverwritePlaycount, "Overwrite existing playcounts", nullptr,
                          CollectionSettings::kDefaultOverwritePlaycount);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kOverwriteRating, "Overwrite existing ratings", nullptr,
                          CollectionSettings::kDefaultOverwriteRating);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kDeleteFiles, "Allow deleting files from collection", nullptr,
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
    GtkWidget *add = gtk_button_new_with_label(Translations::CStr("Add folder…"));
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
