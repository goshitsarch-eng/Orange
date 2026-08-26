#include "widgets/resizabletextedit.h"

ResizableTextEdit::ResizableTextEdit() {
  view_ = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view_), GTK_WRAP_WORD_CHAR);
  widget_ = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), view_);
  gtk_widget_set_size_request(widget_, -1, 80);
}

void ResizableTextEdit::SetText(const std::string &text) {
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_));
  gtk_text_buffer_set_text(buffer, text.c_str(), static_cast<int>(text.size()));
}

std::string ResizableTextEdit::Text() const {
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_));
  GtkTextIter start;
  GtkTextIter end;
  gtk_text_buffer_get_bounds(buffer, &start, &end);
  gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
  std::string out = text ? text : "";
  g_free(text);
  return out;
}
