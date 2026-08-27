#include "clickablelabel.h"

ClickableLabel::ClickableLabel(const std::string &text) {
  button_ = gtk_button_new();
  g_object_ref_sink(button_);
  gtk_widget_add_css_class(button_, "flat");
  gtk_widget_add_css_class(button_, "link");
  label_ = gtk_label_new(text.c_str());
  gtk_button_set_child(GTK_BUTTON(button_), label_);
  g_signal_connect(button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer self) {
                     auto *w = static_cast<ClickableLabel *>(self);
                     if (w->clicked_cb_) w->clicked_cb_();
                   }),
                   this);
}

ClickableLabel::~ClickableLabel() {
  if (button_) g_object_unref(button_);
}

void ClickableLabel::SetText(const std::string &text) {
  gtk_label_set_text(GTK_LABEL(label_), text.c_str());
}

std::string ClickableLabel::text() const {
  const char *t = gtk_label_get_text(GTK_LABEL(label_));
  return t ? t : "";
}
