#include "covermanager/albumcoversearcher.h"

#include "core/application.h"
#include "dialogs/dialoghelpers.h"

#include <adwaita.h>

using DialogHelpers::ApplyCover;
using DialogHelpers::PrettyBytes;
using DialogHelpers::SetImageFromBytes;
using DialogHelpers::SongForDialog;

void AlbumCoverSearcher::Show(GtkWindow *parent, Application *app) {
  Song song = SongForDialog(app);
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Cover search");
  adw_dialog_set_content_width(dialog, 480);
  adw_dialog_set_content_height(dialog, 560);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  GtkWidget *status = gtk_label_new(("Searching providers for “" + song.album() + "”…").c_str());
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  gtk_box_append(GTK_BOX(box), status);
  gtk_box_append(GTK_BOX(box), scroll);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(dialog), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_object_set_data(G_OBJECT(dialog), "list", list);
  g_object_set_data(G_OBJECT(dialog), "status", status);
  g_object_set_data(G_OBJECT(dialog), "count", GINT_TO_POINTER(0));
  app->cover_providers()->FetchAll(song, [dialog, app](const std::string &provider, const std::string &image) {
    if (image.empty() || !GTK_IS_WIDGET(dialog)) {
      return;
    }
    GtkWidget *list = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "list"));
    GtkWidget *status = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "status"));
    auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(dialog), "song"));
    const int count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "count")) + 1;
    g_object_set_data(G_OBJECT(dialog), "count", GINT_TO_POINTER(count));
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), provider.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), (PrettyBytes(static_cast<int64_t>(image.size())) + " image").c_str());
    GtkWidget *thumb = gtk_image_new();
    SetImageFromBytes(thumb, std::vector<unsigned char>(image.begin(), image.end()), 48);
    adw_action_row_add_prefix(ADW_ACTION_ROW(row), thumb);
    GtkWidget *apply = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(apply, "suggested-action");
    auto *image_copy = new std::string(image);
    g_object_set_data_full(G_OBJECT(apply), "image", image_copy, [](gpointer p) { delete static_cast<std::string *>(p); });
    g_object_set_data(G_OBJECT(apply), "song", song);
    g_signal_connect(apply, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(button), "song"));
                       auto *image = static_cast<std::string *>(g_object_get_data(G_OBJECT(button), "image"));
                       if (song && image && ApplyCover(application, song, *image)) {
                         gtk_button_set_label(button, "Saved");
                       } else {
                         gtk_button_set_label(button, "Failed");
                       }
                     })),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
    gtk_label_set_text(GTK_LABEL(status), (std::to_string(count) + " covers found").c_str());
  });
}
