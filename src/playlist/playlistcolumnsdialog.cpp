#include "config.h"
#include "dialogs/dialogchrome.h"

#include "playlist/playlistcolumnsdialog.h"

#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistmoodcolumn.h"
#include "translations/translations.h"

#include <adwaita.h>
#include <string>
#include <vector>

void PlaylistColumnsDialog::Show(GtkWindow *parent, const std::function<void()> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Playlist columns"));
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);

  GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  auto rebuild_list = [](GtkWidget *list_box) {
    GtkWidget *child = gtk_widget_get_first_child(list_box);
    while (child) {
      GtkWidget *next = gtk_widget_get_next_sibling(child);
      gtk_widget_unparent(child);
      child = next;
    }
    std::vector<PlaylistColumn> visible = PlaylistColumnLayout::Visible();
    auto add_row = [&](PlaylistColumn column, bool enabled) {
      GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
      GtkWidget *check = gtk_check_button_new_with_label(PlaylistDelegates::ColumnTitle(column).c_str());
      gtk_check_button_set_active(GTK_CHECK_BUTTON(check), enabled);
      gtk_widget_set_hexpand(check, TRUE);
      g_object_set_data(G_OBJECT(row), "column", GINT_TO_POINTER(static_cast<int>(column) + 1));
      g_object_set_data(G_OBJECT(row), "check", check);
      GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
      GtkWidget *down = gtk_button_new_from_icon_name("go-down-symbolic");
      gtk_widget_set_sensitive(up, enabled);
      gtk_widget_set_sensitive(down, enabled);
      g_object_set_data(G_OBJECT(up), "list", list_box);
      g_object_set_data(G_OBJECT(down), "list", list_box);
      g_object_set_data(G_OBJECT(up), "row", row);
      g_object_set_data(G_OBJECT(down), "row", row);
      g_signal_connect(up, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                         GtkWidget *list_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "list"));
                         GtkWidget *current = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "row"));
                         GtkWidget *prev = gtk_widget_get_prev_sibling(current);
                         if (!prev) {
                           return;
                         }
                         gtk_box_reorder_child_after(GTK_BOX(list_widget), current, gtk_widget_get_prev_sibling(prev));
                       }),
                       nullptr);
      g_signal_connect(down, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                         GtkWidget *list_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "list"));
                         GtkWidget *current = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "row"));
                         GtkWidget *next = gtk_widget_get_next_sibling(current);
                         if (!next) {
                           return;
                         }
                         gtk_box_reorder_child_after(GTK_BOX(list_widget), current, next);
                       }),
                       nullptr);
      gtk_box_append(GTK_BOX(row), check);
      gtk_box_append(GTK_BOX(row), up);
      gtk_box_append(GTK_BOX(row), down);
      gtk_box_append(GTK_BOX(list_box), row);
    };
    for (PlaylistColumn column : visible) {
      if (PlaylistMoodColumn::ShouldOffer(column)) {
        add_row(column, true);
      }
    }
    for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
      const auto column = static_cast<PlaylistColumn>(i);
      if (PlaylistDelegates::ColumnTitle(column).empty() || PlaylistColumnLayout::IsVisible(column) ||
          !PlaylistMoodColumn::ShouldOffer(column)) {
        continue;
      }
      add_row(column, false);
    }
  };
  rebuild_list(list);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 360);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);

  GtkWidget *stretch = gtk_check_button_new_with_label(Translations::CStr("Stretch columns to fit window"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(stretch), PlaylistColumnLayout::StretchEnabled());
  GtkWidget *rating_lock = gtk_check_button_new_with_label(Translations::CStr("Lock rating"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(rating_lock), PlaylistColumnLayout::RatingLocked());

  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *reset = gtk_button_new_with_label(Translations::CStr("Reset"));
  GtkWidget *apply = gtk_button_new_with_label(Translations::CStr("Apply"));
  gtk_widget_add_css_class(apply, "suggested-action");
  gtk_widget_set_hexpand(reset, TRUE);
  auto *cb = new std::function<void()>(callback);
  g_object_set_data(G_OBJECT(apply), "list", list);
  g_object_set_data(G_OBJECT(apply), "stretch", stretch);
  g_object_set_data(G_OBJECT(apply), "rating-lock", rating_lock);
  g_object_set_data_full(G_OBJECT(apply), "callback", cb, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
  g_object_set_data(G_OBJECT(reset), "list", list);
  g_object_set_data(G_OBJECT(reset), "stretch", stretch);
  g_object_set_data(G_OBJECT(reset), "rating-lock", rating_lock);
  g_signal_connect(reset, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     PlaylistColumnLayout::Reset();
                     GtkWidget *list_box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "list"));
                     GtkWidget *child = gtk_widget_get_first_child(list_box);
                     while (child) {
                       GtkWidget *next = gtk_widget_get_next_sibling(child);
                       gtk_widget_unparent(child);
                       child = next;
                     }
                     std::vector<PlaylistColumn> visible = PlaylistColumnLayout::DefaultVisible();
                     for (PlaylistColumn column : visible) {
                       if (!PlaylistMoodColumn::ShouldOffer(column)) {
                         continue;
                       }
                       GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                       GtkWidget *check = gtk_check_button_new_with_label(PlaylistDelegates::ColumnTitle(column).c_str());
                       gtk_check_button_set_active(GTK_CHECK_BUTTON(check), TRUE);
                       gtk_widget_set_hexpand(check, TRUE);
                       g_object_set_data(G_OBJECT(row), "column", GINT_TO_POINTER(static_cast<int>(column) + 1));
                       g_object_set_data(G_OBJECT(row), "check", check);
                       gtk_box_append(GTK_BOX(row), check);
                       gtk_box_append(GTK_BOX(list_box), row);
                     }
                     for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
                       const auto column = static_cast<PlaylistColumn>(i);
                       if (PlaylistDelegates::ColumnTitle(column).empty() || PlaylistColumnLayout::IsVisible(column) ||
                           !PlaylistMoodColumn::ShouldOffer(column)) {
                         continue;
                       }
                       GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                       GtkWidget *check = gtk_check_button_new_with_label(PlaylistDelegates::ColumnTitle(column).c_str());
                       gtk_check_button_set_active(GTK_CHECK_BUTTON(check), FALSE);
                       gtk_widget_set_hexpand(check, TRUE);
                       g_object_set_data(G_OBJECT(row), "column", GINT_TO_POINTER(i + 1));
                       g_object_set_data(G_OBJECT(row), "check", check);
                       gtk_box_append(GTK_BOX(row), check);
                       gtk_box_append(GTK_BOX(list_box), row);
                     }
                     gtk_check_button_set_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "stretch")),
                                                 PlaylistColumnLayout::StretchEnabled());
                     gtk_check_button_set_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "rating-lock")),
                                                 PlaylistColumnLayout::RatingLocked());
                   }),
                   nullptr);
  g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     GtkWidget *list_box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "list"));
                     std::vector<PlaylistColumn> columns;
                     for (GtkWidget *child = gtk_widget_get_first_child(list_box); child; child = gtk_widget_get_next_sibling(child)) {
                       auto *check = static_cast<GtkWidget *>(g_object_get_data(G_OBJECT(child), "check"));
                       if (!check) {
                         check = gtk_widget_get_first_child(child);
                       }
                       if (!GTK_IS_CHECK_BUTTON(check) || !gtk_check_button_get_active(GTK_CHECK_BUTTON(check))) {
                         continue;
                       }
                       const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "column"));
                       if (stored > 0) {
                         columns.push_back(static_cast<PlaylistColumn>(stored - 1));
                       }
                     }
                     PlaylistColumnLayout::SetVisibleColumns(columns);
                     PlaylistColumnLayout::SetStretchEnabled(
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "stretch"))));
                     PlaylistColumnLayout::SetRatingLocked(
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "rating-lock"))));
                     if (auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(button), "callback"))) {
                       (*fn)();
                     }
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(buttons), reset);
  gtk_box_append(GTK_BOX(buttons), apply);
  gtk_box_append(GTK_BOX(box), scroll);
  gtk_box_append(GTK_BOX(box), stretch);
  gtk_box_append(GTK_BOX(box), rating_lock);
  gtk_box_append(GTK_BOX(box), buttons);
  DialogChrome::SetContent(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
