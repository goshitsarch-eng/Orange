#include "collection/savedgroupingmanager.h"

#include "collection/savedgroupinglabels.h"
#include "translations/translations.h"

#include <adwaita.h>

void SavedGroupingManager::Show(GtkWindow *parent, const std::function<void(const CollectionGrouping::Grouping &)> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(SavedGroupingLabels::Title()));
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 360);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(header, 8);
  gtk_widget_set_margin_end(header, 8);
  auto add_header = [](GtkWidget *row, const char *text, bool expand) {
    GtkWidget *label = gtk_label_new(Translations::CStr(text));
    gtk_widget_add_css_class(label, "heading");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    if (expand) {
      gtk_widget_set_hexpand(label, TRUE);
    }
    gtk_box_append(GTK_BOX(row), label);
  };
  add_header(header, SavedGroupingLabels::Name(), true);
  add_header(header, SavedGroupingLabels::FirstLevel(), false);
  add_header(header, SavedGroupingLabels::SecondLevel(), false);
  add_header(header, SavedGroupingLabels::ThirdLevel(), false);
  gtk_box_append(GTK_BOX(box), header);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  auto *cb = new std::function<void(const CollectionGrouping::Grouping &)>(callback);
  g_object_set_data_full(G_OBJECT(dialog), "callback", cb, [](gpointer p) {
    delete static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(p);
  });
  for (const auto &entry : CollectionGrouping::LoadSaved()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), entry.first.c_str());
    const std::string subtitle = CollectionGrouping::Label(entry.second.first) + " / " + CollectionGrouping::Label(entry.second.second) +
                                 " / " + CollectionGrouping::Label(entry.second.third);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle.c_str());
    GtkWidget *apply = gtk_button_new_with_label(Translations::CStr(SavedGroupingLabels::Apply()));
    GtkWidget *remove = gtk_button_new_with_label(Translations::CStr(SavedGroupingLabels::Remove()));
    auto *grouping = new CollectionGrouping::Grouping(entry.second);
    g_object_set_data_full(G_OBJECT(row), "name", g_strdup(entry.first.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(apply), "grouping", grouping, [](gpointer p) { delete static_cast<CollectionGrouping::Grouping *>(p); });
    g_object_set_data(G_OBJECT(apply), "callback", cb);
    g_object_set_data(G_OBJECT(apply), "dialog", dialog);
    g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                       auto *fn = static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(
                           g_object_get_data(G_OBJECT(button), "callback"));
                       auto *grouping = static_cast<CollectionGrouping::Grouping *>(g_object_get_data(G_OBJECT(button), "grouping"));
                       if (fn && grouping) {
                         (*fn)(*grouping);
                       }
                       adw_dialog_close(ADW_DIALOG(g_object_get_data(G_OBJECT(button), "dialog")));
                     }),
                     nullptr);
    g_object_set_data(G_OBJECT(remove), "row", row);
    g_object_set_data(G_OBJECT(remove), "list", list);
    g_signal_connect(remove, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                       GtkWidget *row = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "row"));
                       const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "name"));
                       CollectionGrouping::RemoveSaved(name ? name : "");
                       gtk_list_box_remove(GTK_LIST_BOX(g_object_get_data(G_OBJECT(button), "list")), row);
                     }),
                     nullptr);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), remove);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  }
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  gtk_box_append(GTK_BOX(box), scroll);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
