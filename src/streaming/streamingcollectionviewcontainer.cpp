#include "streaming/streamingcollectionviewcontainer.h"

StreamingCollectionViewContainer::StreamingCollectionViewContainer(const std::string &title)
    : view_(std::make_unique<StreamingCollectionView>(title)) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  progress_ = gtk_progress_bar_new();
  gtk_widget_set_margin_start(progress_, 8);
  gtk_widget_set_margin_end(progress_, 8);
  gtk_widget_set_visible(progress_, FALSE);
  status_ = gtk_label_new("");
  gtk_widget_set_halign(status_, GTK_ALIGN_START);
  gtk_widget_set_margin_start(status_, 8);
  gtk_widget_set_margin_end(status_, 8);
  gtk_widget_set_visible(status_, FALSE);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  gtk_box_append(GTK_BOX(widget_), progress_);
  gtk_box_append(GTK_BOX(widget_), status_);
}

void StreamingCollectionViewContainer::ShowProgress(const std::string &status) {
  if (!status.empty()) {
    SetProgressStatus(status);
  }
  if (progress_) {
    if (!gtk_widget_get_visible(progress_)) {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    }
    gtk_widget_set_visible(progress_, TRUE);
  }
}

void StreamingCollectionViewContainer::HideProgress() {
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    gtk_widget_set_visible(progress_, FALSE);
  }
  if (status_) {
    gtk_label_set_text(GTK_LABEL(status_), "");
    gtk_widget_set_visible(status_, FALSE);
  }
}

void StreamingCollectionViewContainer::SetProgress(int value, int maximum) {
  if (maximum > 0) {
    progress_max_ = maximum;
  }
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), StreamingProgress::Fraction(value, progress_max_));
    gtk_widget_set_visible(progress_, TRUE);
  }
}

void StreamingCollectionViewContainer::SetProgressMaximum(int maximum) {
  if (maximum > 0) {
    progress_max_ = maximum;
  }
}

void StreamingCollectionViewContainer::SetProgressStatus(const std::string &status) {
  if (status_) {
    gtk_label_set_text(GTK_LABEL(status_), status.c_str());
    gtk_widget_set_visible(status_, !status.empty());
  }
}
