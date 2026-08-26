#include "widgets/linetextedit.h"

LineTextEdit::LineTextEdit() {
  widget_ = gtk_entry_new();
  g_signal_connect(widget_, "changed", G_CALLBACK(+[](GtkEditable *editable, gpointer data) {
                     auto *self = static_cast<LineTextEdit *>(data);
                     if (self->changed_) {
                       self->changed_(gtk_editable_get_text(editable));
                     }
                   }),
                   this);
}

void LineTextEdit::SetText(const std::string &text) { gtk_editable_set_text(GTK_EDITABLE(widget_), text.c_str()); }

std::string LineTextEdit::Text() const { return gtk_editable_get_text(GTK_EDITABLE(widget_)); }

void LineTextEdit::SetChangedCallback(std::function<void(const std::string &)> callback) { changed_ = std::move(callback); }
