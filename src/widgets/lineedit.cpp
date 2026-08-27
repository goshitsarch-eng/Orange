#include "widgets/lineedit.h"

LineEdit::LineEdit() {
  widget_ = gtk_entry_new();
  g_signal_connect(widget_, "changed", G_CALLBACK(+[](GtkEditable *editable, gpointer data) {
                     auto *self = static_cast<LineEdit *>(data);
                     if (self->changed_) {
                       const char *text = gtk_editable_get_text(editable);
                       self->changed_(text ? text : "");
                     }
                   }),
                   this);
}

void LineEdit::SetText(const std::string &text) { gtk_editable_set_text(GTK_EDITABLE(widget_), text.c_str()); }

std::string LineEdit::Text() const {
  const char *text = gtk_editable_get_text(GTK_EDITABLE(widget_));
  return text ? text : "";
}

void LineEdit::SetPlaceholder(const std::string &text) { gtk_entry_set_placeholder_text(GTK_ENTRY(widget_), text.c_str()); }

void LineEdit::SetChangedCallback(std::function<void(const std::string &)> callback) { changed_ = std::move(callback); }
