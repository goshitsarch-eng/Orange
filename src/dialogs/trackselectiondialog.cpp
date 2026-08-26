#include "dialogs/trackselectiondialog.h"

#include "core/application.h"

#include <adwaita.h>

void TrackSelectionDialog::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Fetch tags");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *status = gtk_label_new("Searching AcoustID and MusicBrainz…");
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_box_append(GTK_BOX(box), status);
  gtk_box_append(GTK_BOX(box), list);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  app->tag_fetcher()->Results.Connect([status, list, app](const SongList &songs) {
    gtk_label_set_text(GTK_LABEL(status), songs.empty() ? "No matches" : (std::to_string(songs.size()) + " matches — click Apply to write tags").c_str());
    for (const Song &song : songs) {
      auto *copy = new Song(song);
      GtkWidget *row = adw_action_row_new();
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), song.PrettyTitle().c_str());
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), (song.artist() + " – " + song.album() + (song.year() > 0 ? " (" + std::to_string(song.year()) + ")" : "")).c_str());
      GtkWidget *apply = gtk_button_new_with_label("Apply");
      gtk_widget_add_css_class(apply, "suggested-action");
      g_object_set_data_full(G_OBJECT(apply), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
      g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                         auto *application = static_cast<Application *>(data);
                         auto *result = static_cast<Song *>(g_object_get_data(G_OBJECT(btn), "song"));
                         if (!result) {
                           return;
                         }
                         application->tagreader()->WriteFile(*result);
                         if (result->id() > 0) {
                           application->collection()->backend()->AddOrUpdateSong(*result);
                         }
                         gtk_button_set_label(btn, "Applied");
                       }),
                       app);
      adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
      gtk_list_box_append(GTK_LIST_BOX(list), row);
    }
  });
  app->tag_fetcher()->Fetch(app->player()->current_song());
}
