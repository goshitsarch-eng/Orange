#include "widgets/tracksliderpopup.h"

#include <algorithm>

TrackSliderPopup::TrackSliderPopup() {
  widget_ = gtk_popover_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 4);
  gtk_widget_set_margin_bottom(box, 4);
  label_ = gtk_label_new("");
  small_label_ = gtk_label_new("");
  gtk_widget_add_css_class(small_label_, "dim-label");
  gtk_box_append(GTK_BOX(box), label_);
  gtk_box_append(GTK_BOX(box), small_label_);
  gtk_popover_set_child(GTK_POPOVER(widget_), box);
  gtk_popover_set_autohide(GTK_POPOVER(widget_), FALSE);
  gtk_popover_set_has_arrow(GTK_POPOVER(widget_), TRUE);
  gtk_widget_set_can_target(widget_, FALSE);
}

TrackSliderPopup::~TrackSliderPopup() {
  if (widget_) {
    gtk_widget_unparent(widget_);
  }
}

void TrackSliderPopup::Attach(GtkWidget *relative) {
  if (!relative) {
    return;
  }
  GtkWidget *parent = gtk_widget_get_parent(widget_);
  if (parent == relative) {
    return;
  }
  if (parent) {
    gtk_widget_unparent(widget_);
  }
  gtk_widget_set_parent(widget_, relative);
}

void TrackSliderPopup::SetLabels(const std::string &text, const std::string &small_text) {
  gtk_label_set_text(GTK_LABEL(label_), text.c_str());
  gtk_label_set_text(GTK_LABEL(small_label_), small_text.c_str());
}

void TrackSliderPopup::ShowText(GtkWidget *relative, const std::string &text) { ShowAt(relative, 0, text, {}); }

void TrackSliderPopup::ShowAt(GtkWidget *relative, int x, const std::string &text, const std::string &small_text) {
  if (!relative) {
    return;
  }
  Attach(relative);
  SetLabels(text, small_text);
  const int height = std::max(1, gtk_widget_get_height(relative));
  GdkRectangle rect{x, height / 2, 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(widget_), &rect);
  gtk_popover_popup(GTK_POPOVER(widget_));
}

void TrackSliderPopup::Hide() {
  if (widget_) {
    gtk_popover_popdown(GTK_POPOVER(widget_));
  }
}
