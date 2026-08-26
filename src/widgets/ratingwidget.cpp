#include "widgets/ratingwidget.h"

RatingWidget::RatingWidget() {
  widget_ = gtk_button_new();
  gtk_widget_add_css_class(widget_, "flat");
  label_ = gtk_label_new("");
  gtk_button_set_child(GTK_BUTTON(widget_), label_);
  Refresh();
  g_signal_connect(widget_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<RatingWidget *>(data);
                     float next = self->rating_ < 0.0f ? 0.2f : self->rating_ + 0.2f;
                     if (next > 1.001f) {
                       next = -1.0f;
                     }
                     self->set_rating(next);
                     if (self->changed_) {
                       self->changed_(self->rating_);
                     }
                   }),
                   this);
}

void RatingWidget::set_rating(float rating) {
  rating_ = rating;
  Refresh();
}

void RatingWidget::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void RatingWidget::Refresh() {
  const std::string stars = RatingPainter::Stars(rating_);
  gtk_label_set_text(GTK_LABEL(label_), stars.empty() ? "☆☆☆☆☆" : stars.c_str());
  gtk_widget_set_tooltip_text(widget_, stars.empty() ? "Unrated" : stars.c_str());
}
