#include "dialogs/saveplaylistsdialog.h"
#include "dialogs/dialogchrome.h"

#include "constants/playlistsettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "dialogs/messagedialog.h"
#include "dialogs/saveplaylistsoptions.h"
#include "playlistparsers/playlistparser.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"

#include <adwaita.h>

#include <vector>

namespace {

struct SaveAllState {
  Application *app = nullptr;
  GtkWindow *parent = nullptr;
  GtkWidget *dialog = nullptr;
  GtkWidget *path_entry = nullptr;
  GtkWidget *type_drop = nullptr;
  std::vector<std::string> extensions;
};

void SaveAllToDirectory(SaveAllState *state) {
  const char *path_text = gtk_editable_get_text(GTK_EDITABLE(state->path_entry));
  const std::string path = path_text ? path_text : "";
  if (path.empty()) {
    return;
  }
  if (!SavePlaylistsOptions::ValidateDirectory(path)) {
    MessageDialog::Show(state->parent, SavePlaylistsOptions::DirectoryMissingTitle(), SavePlaylistsOptions::DirectoryMissingBody());
    return;
  }
  const guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(state->type_drop));
  const std::string extension = selected < state->extensions.size() ? state->extensions[selected] : SavePlaylistsOptions::DefaultExtension({});
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  settings.SetValue(PlaylistSettings::kLastSaveAllPath, path);
  settings.SetValue(PlaylistSettings::kLastSaveAllExtension, extension);
  settings.Sync();
  PlaylistParser parser;
  std::vector<std::string> failed;
  for (const auto &playlist : state->app->playlist_manager()->playlists()) {
    if (!parser.Save(FileUtils::Join(path, SavePlaylistsOptions::DestFilename(playlist->name(), extension)), playlist->songs())) {
      failed.push_back(playlist->name());
    }
  }
  adw_dialog_close(ADW_DIALOG(state->dialog));
  if (!failed.empty()) {
    // Silently dropping the write left the user believing every playlist had been saved.
    MessageDialog::Show(state->parent, SavePlaylistsOptions::SaveFailedTitle(),
                        SavePlaylistsOptions::SaveFailedBody(failed).c_str());
  }
}

}  // namespace

void SavePlaylistsDialog::Show(GtkWindow *parent, Application *app) {
  auto *state = new SaveAllState;
  state->app = app;
  state->parent = parent;
  state->extensions = SavePlaylistsOptions::ExtensionChoices();

  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  const std::string path = SavePlaylistsOptions::FallbackPath(settings.Value(PlaylistSettings::kLastSaveAllPath),
                                                             g_get_home_dir() ? g_get_home_dir() : "");
  const std::string extension = SavePlaylistsOptions::DefaultExtension(settings.Value(PlaylistSettings::kLastSaveAllExtension));

  AdwDialog *dialog = adw_dialog_new();
  state->dialog = GTK_WIDGET(dialog);
  adw_dialog_set_title(dialog, Translations::CStr(SavePlaylistsOptions::Title()));
  adw_dialog_set_content_width(dialog, 440);
  g_object_set_data_full(G_OBJECT(dialog), "state", state, [](gpointer p) { delete static_cast<SaveAllState *>(p); });

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);

  GtkWidget *path_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  state->path_entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(state->path_entry), path.c_str());
  gtk_widget_set_hexpand(state->path_entry, TRUE);
  GtkWidget *browse = gtk_button_new_from_icon_name("folder-symbolic");
  gtk_widget_set_tooltip_text(browse, Translations::CStr(SavePlaylistsOptions::BrowseTitle()));
  gtk_box_append(GTK_BOX(path_row), state->path_entry);
  gtk_box_append(GTK_BOX(path_row), browse);

  GtkWidget *type_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *type_label = gtk_label_new(Translations::CStr(SavePlaylistsOptions::TypeLabel()));
  gtk_widget_set_halign(type_label, GTK_ALIGN_START);
  std::vector<const char *> labels;
  labels.reserve(state->extensions.size() + 1);
  for (const std::string &choice : state->extensions) {
    labels.push_back(choice.c_str());
  }
  labels.push_back(nullptr);
  state->type_drop = gtk_drop_down_new_from_strings(labels.data());
  gtk_drop_down_set_selected(GTK_DROP_DOWN(state->type_drop),
                             static_cast<guint>(SavePlaylistsOptions::ExtensionIndex(state->extensions, extension)));
  gtk_box_append(GTK_BOX(type_row), type_label);
  gtk_box_append(GTK_BOX(type_row), state->type_drop);

  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(buttons, GTK_ALIGN_END);
  GtkWidget *cancel = gtk_button_new_with_label(Translations::CStr("Cancel"));
  GtkWidget *ok = gtk_button_new_with_label(Translations::CStr("OK"));
  gtk_widget_add_css_class(ok, "suggested-action");
  gtk_box_append(GTK_BOX(buttons), cancel);
  gtk_box_append(GTK_BOX(buttons), ok);

  g_signal_connect(browse, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<SaveAllState *>(data);
                     GtkFileDialog *chooser = gtk_file_dialog_new();
                     gtk_file_dialog_set_title(chooser, Translations::CStr(SavePlaylistsOptions::BrowseTitle()));
                     const char *current = gtk_editable_get_text(GTK_EDITABLE(self->path_entry));
                     if (current && *current) {
                       GFile *initial = g_file_new_for_path(current);
                       gtk_file_dialog_set_initial_folder(chooser, initial);
                       g_object_unref(initial);
                     }
                     gtk_file_dialog_select_folder(chooser, self->parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
                       auto *self = static_cast<SaveAllState *>(data);
                       GError *error = nullptr;
                       GFile *folder = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
                       if (!folder) {
                         if (error) {
                           g_error_free(error);
                         }
                         return;
                       }
                       gchar *chosen = g_file_get_path(folder);
                       if (chosen) {
                         gtk_editable_set_text(GTK_EDITABLE(self->path_entry), chosen);
                         g_free(chosen);
                       }
                       g_object_unref(folder);
                     }, self);
                   }),
                   state);
  g_signal_connect(cancel, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     adw_dialog_close(ADW_DIALOG(static_cast<SaveAllState *>(data)->dialog));
                   }),
                   state);
  g_signal_connect(ok, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { SaveAllToDirectory(static_cast<SaveAllState *>(data)); }), state);

  gtk_box_append(GTK_BOX(box), path_row);
  gtk_box_append(GTK_BOX(box), type_row);
  gtk_box_append(GTK_BOX(box), buttons);
  DialogChrome::SetContent(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
