#include "dialogs/deleteconfirmationdialog.h"

#include "core/application.h"
#include "core/deletefiles.h"
#include "core/deletefilesjob.h"
#include "core/filesystemmusicstorage.h"
#include "dialogs/errordialog.h"
#include "organize/organizeerrordialog.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <adwaita.h>

namespace {

struct DeleteJobState {
  Application *app = nullptr;
  GtkWindow *parent = nullptr;
  SongList requested;
  FilesystemMusicStorage storage{""};
  DeleteFiles *deleter = nullptr;
};

void ApplyDeleteFinished(DeleteJobState *state, const SongList &errors) {
  if (!state || !state->app) {
    return;
  }
  if (Playlist *playlist = state->app->playlist_manager() ? state->app->playlist_manager()->current() : nullptr) {
    const std::vector<int> rows = DeleteFilesJob::RowsToRemove(playlist->songs(), state->requested, errors);
    if (!rows.empty()) {
      playlist->RemoveRows(rows);
      state->app->playlist_manager()->SaveCurrent();
    }
  }
  if (state->app->collection()) {
    state->app->collection()->IncrementalScan();
  }
  if (!errors.empty()) {
    OrganizeErrorDialog::Show(state->parent, OrganizeErrorDialog::OperationType::Delete, errors);
  }
}

gboolean DeleteJobStateFree(gpointer data) {
  auto *state = static_cast<DeleteJobState *>(data);
  delete state->deleter;
  delete state;
  return G_SOURCE_REMOVE;
}

}  // namespace

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
  adw_alert_dialog_set_default_response(dialog, "cancel");
  adw_alert_dialog_set_close_response(dialog, "cancel");
  auto *owned = new SongList(targets);
  g_object_set_data_full(G_OBJECT(dialog), "songs", owned, [](gpointer p) { delete static_cast<SongList *>(p); });
  g_object_set_data(G_OBJECT(dialog), "parent", parent);
  g_signal_connect(dialog, "response", G_CALLBACK((+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "delete") != 0) {
                       return;
                     }
                     auto *application = static_cast<Application *>(data);
                     auto *list = static_cast<SongList *>(g_object_get_data(G_OBJECT(alert), "songs"));
                     if (!list || !application) {
                       return;
                     }
                     auto *state = new DeleteJobState;
                     state->app = application;
                     state->parent = GTK_WINDOW(g_object_get_data(G_OBJECT(alert), "parent"));
                     state->requested = *list;
                     state->deleter = new DeleteFiles(application->task_manager(), &state->storage, true);
                     state->deleter->Finished.Connect([state](const SongList &errors) {
                       ApplyDeleteFinished(state, errors);
                       g_idle_add(DeleteJobStateFree, state);
                     });
                     state->deleter->StartAsync(state->requested);
                   })),
                   app);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
