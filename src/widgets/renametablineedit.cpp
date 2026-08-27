#include "widgets/renametablineedit.h"

RenameTabLineEdit::RenameTabLineEdit() {
  widget_ = gtk_entry_new();
  g_signal_connect(widget_, "activate", G_CALLBACK(+[](GtkEntry *entry, gpointer data) {
                     auto *self = static_cast<RenameTabLineEdit *>(data);
                     if (self->accepted_) {
                       self->accepted_(gtk_editable_get_text(GTK_EDITABLE(entry)));
                     }
                   }),
                   this);
}

void RenameTabLineEdit::SetText(const std::string &text) { gtk_editable_set_text(GTK_EDITABLE(widget_), text.c_str()); }

std::string RenameTabLineEdit::Text() const { return gtk_editable_get_text(GTK_EDITABLE(widget_)); }

void RenameTabLineEdit::SetAcceptedCallback(std::function<void(const std::string &)> callback) { accepted_ = std::move(callback); }
