#include "smartplaylists/smartplaylistwizardfinishpage.h"

SmartPlaylistWizardFinishPage::SmartPlaylistWizardFinishPage() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  summary_ = gtk_label_new("Ready to create the playlist.");
  gtk_label_set_wrap(GTK_LABEL(summary_), TRUE);
  gtk_widget_set_halign(summary_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new("Finish"));
  gtk_box_append(GTK_BOX(widget_), summary_);
}

void SmartPlaylistWizardFinishPage::SetSummary(const std::string &text) {
  gtk_label_set_text(GTK_LABEL(summary_), text.c_str());
}
