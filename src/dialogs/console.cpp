#include "dialogs/console.h"
#include "dialogs/dialogchrome.h"

#include "dialogs/consolequery.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <string>

namespace {

void AppendText(GtkTextBuffer *buffer, const std::string &text) {
  GtkTextIter end;
  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_insert(buffer, &end, text.c_str(), -1);
}

void ScrollToEnd(GtkTextView *view) {
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
  GtkTextIter end;
  gtk_text_buffer_get_end_iter(buffer, &end);
  GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, nullptr, &end, FALSE);
  gtk_text_view_scroll_mark_onscreen(view, mark);
  gtk_text_buffer_delete_mark(buffer, mark);
}

void RunClicked(GtkEntry *entry, GtkTextView *view, Database *database) {
  const char *sql = gtk_editable_get_text(GTK_EDITABLE(entry));
  if (!sql || !*sql) {
    return;
  }
  const ConsoleQuery::Result result = ConsoleQuery::Run(database, sql);
  AppendText(gtk_text_view_get_buffer(view), ConsoleQuery::Format(result));
  ScrollToEnd(view);
}

}  // namespace

void Console::Show(GtkWindow *parent, Database *database) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Debug console"));
  adw_dialog_set_content_width(dialog, 760);
  adw_dialog_set_content_height(dialog, 520);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), Translations::CStr("SQL query"));
  GtkWidget *run = gtk_button_new_with_label(Translations::CStr("Run"));
  gtk_widget_add_css_class(run, "suggested-action");
  gtk_box_append(GTK_BOX(row), entry);
  gtk_box_append(GTK_BOX(row), run);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);

  gtk_box_append(GTK_BOX(box), row);
  gtk_box_append(GTK_BOX(box), scroll);
  DialogChrome::SetContent(dialog, box);

  struct State {
    Database *database = nullptr;
    GtkEntry *entry = nullptr;
    GtkTextView *view = nullptr;
  };
  auto *state = new State{database, GTK_ENTRY(entry), GTK_TEXT_VIEW(view)};
  g_object_set_data_full(G_OBJECT(dialog), "console-state", state, +[](gpointer data) { delete static_cast<State *>(data); });

  g_signal_connect(run, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *s = static_cast<State *>(data);
                     RunClicked(s->entry, s->view, s->database);
                   }),
                   state);
  g_signal_connect(entry, "activate", G_CALLBACK(+[](GtkEntry *, gpointer data) {
                     auto *s = static_cast<State *>(data);
                     RunClicked(s->entry, s->view, s->database);
                   }),
                   state);

  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
