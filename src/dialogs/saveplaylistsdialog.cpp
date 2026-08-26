#include "dialogs/saveplaylistsdialog.h"

#include "core/application.h"
#include "playlistparsers/playlistparser.h"
#include "utilities/fileutils.h"

void SavePlaylistsDialog::Show(GtkWindow *parent, Application *app) {
  GtkFileDialog *chooser = gtk_file_dialog_new();
  gtk_file_dialog_set_title(chooser, "Save all playlists");
  gtk_file_dialog_select_folder(chooser, parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *application = static_cast<Application *>(data);
    GError *error = nullptr;
    GFile *folder = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!folder) {
      if (error) {
        g_error_free(error);
      }
      return;
    }
    gchar *path = g_file_get_path(folder);
    if (path) {
      for (const auto &playlist : application->playlist_manager()->playlists()) {
        const std::string dest = FileUtils::Join(path, playlist->name() + ".m3u");
        PlaylistParser().Save(dest, playlist->songs());
      }
      g_free(path);
    }
    g_object_unref(folder);
  }, app);
}
