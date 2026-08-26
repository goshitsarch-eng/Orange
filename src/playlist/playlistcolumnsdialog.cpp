#include "playlist/playlistcolumnsdialog.h"

#include "core/settings.h"
#include "playlist/playlistdelegates.h"
#include "utilities/strutils.h"

#include <adwaita.h>
#include <string>
#include <vector>

void PlaylistColumnsDialog::Show(GtkWindow *parent, const std::function<void()> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Playlist columns");
  adw_dialog_set_content_width(dialog, 360);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  Settings settings;
  settings.BeginGroup("Playlist");
  const std::string enabled = settings.Value("columns", "Track,Title,Artist,Album,Album artist,Length,Year,Genre,Bitrate,Sample rate,Plays,Rating,Filename");
  const std::vector<std::string> enabled_parts = StrUtils::Split(enabled, ',');
  auto is_enabled = [&enabled_parts](const std::string &title) {
    for (const std::string &part : enabled_parts) {
      if (part == title) {
        return true;
      }
    }
    return false;
  };
  GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
    const std::string title = PlaylistDelegates::ColumnTitle(static_cast<PlaylistColumn>(i));
    if (title.empty()) {
      continue;
    }
    GtkWidget *check = gtk_check_button_new_with_label(title.c_str());
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), is_enabled(title));
    gtk_box_append(GTK_BOX(list), check);
  }
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 360);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  GtkWidget *apply = gtk_button_new_with_label("Apply");
  gtk_widget_add_css_class(apply, "suggested-action");
  auto *cb = new std::function<void()>(callback);
  g_object_set_data(G_OBJECT(apply), "list", list);
  g_object_set_data_full(G_OBJECT(apply), "callback", cb, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
  g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     GtkWidget *list_box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "list"));
                     std::string value;
                     for (GtkWidget *child = gtk_widget_get_first_child(list_box); child; child = gtk_widget_get_next_sibling(child)) {
                       if (GTK_IS_CHECK_BUTTON(child) && gtk_check_button_get_active(GTK_CHECK_BUTTON(child))) {
                         if (!value.empty()) {
                           value += ",";
                         }
                         value += gtk_check_button_get_label(GTK_CHECK_BUTTON(child));
                       }
                     }
                     Settings settings;
                     settings.BeginGroup("Playlist");
                     settings.SetValue("columns", value);
                     settings.Sync();
                     if (auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(button), "callback"))) {
                       (*fn)();
                     }
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), scroll);
  gtk_box_append(GTK_BOX(box), apply);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
