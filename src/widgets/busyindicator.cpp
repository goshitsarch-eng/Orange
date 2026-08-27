#include "busyindicator.h"

BusyIndicator::BusyIndicator() {
  root_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  g_object_ref_sink(root_);
  spinner_ = gtk_spinner_new();
  label_ = gtk_label_new("");
  gtk_box_append(GTK_BOX(root_), spinner_);
  gtk_box_append(GTK_BOX(root_), label_);
  Hide();
}

BusyIndicator::~BusyIndicator() {
  if (root_) g_object_unref(root_);
}

void BusyIndicator::Show(const std::string &text) {
  visible_ = true;
  gtk_label_set_text(GTK_LABEL(label_), text.empty() ? "Working…" : text.c_str());
  gtk_spinner_start(GTK_SPINNER(spinner_));
  gtk_widget_set_visible(root_, TRUE);
}

void BusyIndicator::Hide() {
  visible_ = false;
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  gtk_widget_set_visible(root_, FALSE);
}
