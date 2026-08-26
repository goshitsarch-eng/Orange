#include "widgets/tracksliderpopup.h"

TrackSliderPopup::TrackSliderPopup() {
  widget_ = gtk_popover_new();
  label_ = gtk_label_new("");
  gtk_widget_set_margin_start(label_, 8);
  gtk_widget_set_margin_end(label_, 8);
  gtk_widget_set_margin_top(label_, 4);
  gtk_widget_set_margin_bottom(label_, 4);
  gtk_popover_set_child(GTK_POPOVER(widget_), label_);
  gtk_popover_set_autohide(GTK_POPOVER(widget_), FALSE);
  gtk_popover_set_has_arrow(GTK_POPOVER(widget_), FALSE);
}

TrackSliderPopup::~TrackSliderPopup() {
  if (widget_) {
    gtk_widget_unparent(widget_);
  }
}

void TrackSliderPopup::ShowText(GtkWidget *relative, const std::string &text) {
  if (!relative) {
    return;
  }
  gtk_widget_set_parent(widget_, relative);
  gtk_label_set_text(GTK_LABEL(label_), text.c_str());
  gtk_popover_popup(GTK_POPOVER(widget_));
}

void TrackSliderPopup::Hide() {
  if (widget_) {
    gtk_popover_popdown(GTK_POPOVER(widget_));
  }
}
