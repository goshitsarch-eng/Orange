#include "dialogs/deleteconfirmationdialog.h"

#include "core/application.h"
#include "dialogs/errordialog.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <adwaita.h>

void DeleteConfirmationDialog::Show(GtkWindow *parent, Application *app) {
  Playlist *playlist = app->playlist_manager()->active();
  if (!playlist || playlist->current_row() < 0) {
    ErrorDialog::Show(parent, Translations::Tr("Select a song in the playlist first."));
    return;
  }
  const Song song = playlist->current_song();
  const std::string body = StrUtils::Replace(Translations::Tr("Permanently delete “%1” from disk?"), "%1", song.PrettyTitle());
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr("Delete files"), body.c_str()));
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "delete", Translations::CStr("Delete"), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "delete", ADW_RESPONSE_DESTRUCTIVE);
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(dialog), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "delete") != 0) {
                       return;
                     }
                     auto *application = static_cast<Application *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(alert), "song"));
                     const std::string path = FileUtils::PathFromUri(song->url());
                     GFile *file = g_file_new_for_path(path.c_str());
                     if (!g_file_trash(file, nullptr, nullptr)) {
                       FileUtils::Remove(path);
                     }
                     g_object_unref(file);
                     if (application->playlist_manager()->active()) {
                       application->playlist_manager()->active()->RemoveRows({application->playlist_manager()->current_row()});
                       application->playlist_manager()->SaveActive();
                     }
                   }),
                   app);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
