#include "collection/groupbydialog.h"

#include "collection/groupbydialoglabels.h"
#include "collection/savedgroupingmanager.h"
#include "translations/translations.h"

#include <adwaita.h>

void GroupByDialog::Show(GtkWindow *parent, const CollectionGrouping::Grouping &current,
                         const std::function<void(const CollectionGrouping::Grouping &)> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(GroupByDialogLabels::Title()));
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  GtkWidget *first = gtk_drop_down_new_from_strings(CollectionGrouping::kComboLabels);
  GtkWidget *second = gtk_drop_down_new_from_strings(CollectionGrouping::kComboLabels);
  GtkWidget *third = gtk_drop_down_new_from_strings(CollectionGrouping::kComboLabels);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(first), CollectionGrouping::ComboIndex(current.first));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(second), CollectionGrouping::ComboIndex(current.second));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(third), CollectionGrouping::ComboIndex(current.third));
  GtkWidget *separate = gtk_check_button_new_with_label("Separate albums by grouping tag");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(separate), CollectionGrouping::SeparateAlbumsByGrouping());
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Save as…");
  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *apply = gtk_button_new_with_label(Translations::CStr("Apply"));
  gtk_widget_add_css_class(apply, "suggested-action");
  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save"));
  GtkWidget *manage = gtk_button_new_with_label(Translations::CStr("Manage"));
  gtk_box_append(GTK_BOX(buttons), apply);
  gtk_box_append(GTK_BOX(buttons), save);
  gtk_box_append(GTK_BOX(buttons), manage);
  auto *cb = new std::function<void(const CollectionGrouping::Grouping &)>(callback);
  g_object_set_data_full(G_OBJECT(dialog), "callback", cb, [](gpointer p) {
    delete static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(p);
  });
  g_object_set_data(G_OBJECT(dialog), "first", first);
  g_object_set_data(G_OBJECT(dialog), "second", second);
  g_object_set_data(G_OBJECT(dialog), "third", third);
  g_object_set_data(G_OBJECT(dialog), "separate", separate);
  g_object_set_data(G_OBJECT(dialog), "name", name);
  g_object_set_data(G_OBJECT(apply), "dialog", dialog);
  g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *fn = static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(data);
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "dialog"));
                     CollectionGrouping::Grouping grouping;
                     grouping.first = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "first")))));
                     grouping.second = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "second")))));
                     grouping.third = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "third")))));
                     CollectionGrouping::SetSeparateAlbumsByGrouping(
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(dlg), "separate"))));
                     (*fn)(grouping);
                     adw_dialog_close(ADW_DIALOG(dlg));
                   }),
                   cb);
  g_object_set_data(G_OBJECT(save), "dialog", dialog);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "dialog"));
                     const char *title = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "name")));
                     CollectionGrouping::Grouping grouping;
                     grouping.first = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "first")))));
                     grouping.second = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "second")))));
                     grouping.third = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "third")))));
                     CollectionGrouping::AddSaved(title ? title : "", grouping);
                   }),
                   nullptr);
  g_object_set_data(G_OBJECT(manage), "parent", parent);
  g_signal_connect(manage, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *fn = static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(data);
                     SavedGroupingManager::Show(GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), *fn);
                   }),
                   cb);
  GtkWidget *intro = gtk_label_new(Translations::CStr(GroupByDialogLabels::Intro()));
  gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
  gtk_widget_set_halign(intro, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), intro);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(GroupByDialogLabels::GroupBy())));
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(GroupByDialogLabels::FirstLevel())));
  gtk_box_append(GTK_BOX(box), first);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(GroupByDialogLabels::SecondLevel())));
  gtk_box_append(GTK_BOX(box), second);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(GroupByDialogLabels::ThirdLevel())));
  gtk_box_append(GTK_BOX(box), third);
  gtk_box_append(GTK_BOX(box), separate);
  gtk_box_append(GTK_BOX(box), name);
  gtk_box_append(GTK_BOX(box), buttons);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
