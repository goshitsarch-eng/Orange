#include "dialogs/deleteconfirmationdialog.h"

#include "core/application.h"
#include "core/deletefiles.h"
#include "dialogs/errordialog.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <adwaita.h>

void DeleteConfirmationDialog::Show(GtkWindow *parent, Application *app, const SongList &songs, DeleteFilesPolicy::Source source) {
  if (!DeleteFilesPolicy::Allowed(source)) {
    ErrorDialog::Show(parent, Translations::CStr(DeleteFilesPolicy::DeniedMessage(source)));
    return;
  }
  SongList targets = songs;
  if (targets.empty()) {
    Playlist *playlist = app->playlist_manager()->current();
    if (playlist && playlist->current_row() >= 0) {
      targets.push_back(playlist->current_song());
    }
  }
  if (targets.empty()) {
    ErrorDialog::Show(parent, Translations::Tr("Select a song first."));
    return;
  }
  const std::string body = targets.size() == 1
                               ? StrUtils::Replace(Translations::Tr("Permanently delete “%1” from disk?"), "%1", targets.front().PrettyTitle())
                               : StrUtils::Replace(Translations::Tr("Permanently delete %1 files from disk?"), "%1", std::to_string(targets.size()));
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr("Delete files"), body.c_str()));
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "delete", Translations::CStr("Delete"), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "delete", ADW_RESPONSE_DESTRUCTIVE);
  auto *owned = new SongList(targets);
  g_object_set_data_full(G_OBJECT(dialog), "songs", owned, [](gpointer p) { delete static_cast<SongList *>(p); });
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "delete") != 0) {
                       return;
                     }
                     auto *application = static_cast<Application *>(data);
                     auto *list = static_cast<SongList *>(g_object_get_data(G_OBJECT(alert), "songs"));
                     if (!list) {
                       return;
                     }
                     DeleteFiles deleter(application->task_manager(), nullptr, true);
                     deleter.Start(*list);
                     if (application->playlist_manager()->current()) {
                       std::vector<int> rows;
                       const SongList playlist_songs = application->playlist_manager()->current()->songs();
                       for (size_t i = 0; i < playlist_songs.size(); ++i) {
                         for (const Song &song : *list) {
                           if (playlist_songs[i].url() == song.url()) {
                             rows.push_back(static_cast<int>(i));
                             break;
                           }
                         }
                       }
                       if (!rows.empty()) {
                         application->playlist_manager()->current()->RemoveRows(rows);
                         application->playlist_manager()->SaveCurrent();
                       }
                     }
                     application->collection()->IncrementalScan();
                   }),
                   app);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
