#include "equalizer/equalizerdialog.h"

#include "equalizer/equalizer.h"
#include "translations/translations.h"

#include <adwaita.h>

void EqualizerDialog::Show(GtkWindow *parent, Equalizer *equalizer) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Equalizer"));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *enable = gtk_check_button_new_with_label(Translations::CStr("Enable equalizer"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(enable), equalizer->enabled());
  g_signal_connect(enable, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     static_cast<class Equalizer *>(data)->set_enabled(gtk_check_button_get_active(button));
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), enable);
  GtkStringList *preset_names = gtk_string_list_new(nullptr);
  for (const std::string &name : equalizer->Presets()) {
    gtk_string_list_append(preset_names, name.c_str());
  }
  GtkWidget *preset = gtk_drop_down_new(G_LIST_MODEL(preset_names), nullptr);
  g_signal_connect(preset, "notify::selected", G_CALLBACK(+[](GtkDropDown *drop, GParamSpec *, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkStringObject *item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop));
                     if (item) {
                       eq->LoadPreset(gtk_string_object_get_string(item));
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), preset);
  GtkWidget *preset_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *preset_name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(preset_name), "Custom preset name");
  GtkWidget *save_preset = gtk_button_new_with_label("Save");
  GtkWidget *delete_preset = gtk_button_new_with_label("Delete");
  g_object_set_data(G_OBJECT(save_preset), "name", preset_name);
  g_object_set_data(G_OBJECT(save_preset), "list", preset_names);
  g_signal_connect(save_preset, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkWidget *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "name"));
                     const char *name = gtk_editable_get_text(GTK_EDITABLE(entry));
                     if (eq->SavePreset(name ? name : "")) {
                       gtk_string_list_append(GTK_STRING_LIST(g_object_get_data(G_OBJECT(button), "list")), name);
                     }
                   }),
                   equalizer);
  g_object_set_data(G_OBJECT(delete_preset), "drop", preset);
  g_signal_connect(delete_preset, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkDropDown *drop = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "drop"));
                     GtkStringObject *item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop));
                     if (item) {
                       eq->DeletePreset(gtk_string_object_get_string(item));
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(preset_row), preset_name);
  gtk_box_append(GTK_BOX(preset_row), save_preset);
  gtk_box_append(GTK_BOX(preset_row), delete_preset);
  gtk_box_append(GTK_BOX(box), preset_row);
  GtkWidget *preamp = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -12, 12, 1);
  gtk_range_set_value(GTK_RANGE(preamp), equalizer->preamp());
  gtk_widget_set_tooltip_text(preamp, "Preamp");
  g_signal_connect(preamp, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     static_cast<class Equalizer *>(data)->set_preamp(static_cast<int>(gtk_range_get_value(range)));
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), preamp);
  GtkWidget *bands = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  static const char *kHz[] = {"60", "170", "310", "600", "1k", "3k", "6k", "12k", "14k", "16k"};
  for (int i = 0; i < 10; ++i) {
    GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -12, 12, 1);
    gtk_range_set_inverted(GTK_RANGE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), equalizer->gains()[static_cast<size_t>(i)]);
    gtk_widget_set_size_request(scale, -1, 160);
    g_object_set_data(G_OBJECT(scale), "band", GINT_TO_POINTER(i));
    g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                       const int band = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(range), "band"));
                       static_cast<class Equalizer *>(data)->set_gain(band, static_cast<int>(gtk_range_get_value(range)));
                     }),
                     equalizer);
    gtk_box_append(GTK_BOX(col), scale);
    gtk_box_append(GTK_BOX(col), gtk_label_new(kHz[i]));
    gtk_box_append(GTK_BOX(bands), col);
  }
  gtk_box_append(GTK_BOX(box), bands);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
