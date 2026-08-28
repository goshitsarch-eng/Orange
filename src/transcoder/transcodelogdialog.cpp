#include "transcoder/transcodelogdialog.h"
#include "dialogs/dialogchrome.h"

#include "transcoder/transcodelog.h"
#include "translations/translations.h"

#include <adwaita.h>

void TranscodeLogDialog::Show(GtkWindow *parent, std::vector<std::string> *lines, GtkWidget **view_slot, GtkWidget *preview) {
  if (view_slot && *view_slot) {
    if (lines) {
      gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(*view_slot)), TranscodeLog::Join(*lines).c_str(), -1);
    }
    if (auto *existing = ADW_DIALOG(g_object_get_data(G_OBJECT(*view_slot), "dialog"))) {
      adw_dialog_present(existing, GTK_WIDGET(parent));
    }
    return;
  }

  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Transcoder Log"));
  adw_dialog_set_content_width(dialog, 680);
  adw_dialog_set_content_height(dialog, 360);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);

  GtkWidget *view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
  if (lines) {
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), TranscodeLog::Join(*lines).c_str(), -1);
  }
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
  gtk_box_append(GTK_BOX(box), scroll);

  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(buttons, GTK_ALIGN_END);
  GtkWidget *clear = gtk_button_new_with_label(Translations::CStr("Clear"));
  GtkWidget *close = gtk_button_new_with_label(Translations::CStr("Close"));
  gtk_box_append(GTK_BOX(buttons), clear);
  gtk_box_append(GTK_BOX(buttons), close);
  gtk_box_append(GTK_BOX(box), buttons);

  g_object_set_data(G_OBJECT(clear), "view", view);
  g_object_set_data(G_OBJECT(clear), "lines", lines);
  g_object_set_data(G_OBJECT(clear), "preview", preview);
  g_signal_connect(clear, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                     auto *owned = static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(button), "lines"));
                     TranscodeLog::Clear(owned);
                     auto *text = GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(button), "view"));
                     if (text) {
                       gtk_text_buffer_set_text(gtk_text_view_get_buffer(text), "", -1);
                     }
                     if (auto *label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "preview"))) {
                       gtk_label_set_text(GTK_LABEL(label), "");
                     }
                   })),
                   nullptr);
  g_object_set_data(G_OBJECT(close), "dialog", dialog);
  g_signal_connect(close, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     adw_dialog_close(ADW_DIALOG(g_object_get_data(G_OBJECT(button), "dialog")));
                   }),
                   nullptr);
  g_object_set_data(G_OBJECT(view), "dialog", dialog);
  if (view_slot) {
    *view_slot = view;
    g_object_set_data(G_OBJECT(dialog), "view-slot", view_slot);
    g_signal_connect(dialog, "closed", G_CALLBACK(+[](AdwDialog *dlg, gpointer) {
                       if (auto **slot = static_cast<GtkWidget **>(g_object_get_data(G_OBJECT(dlg), "view-slot"))) {
                         *slot = nullptr;
                       }
                     }),
                     nullptr);
  }

  DialogChrome::SetContent(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
