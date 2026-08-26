#include "smartplaylists/smartplaylistwizardtypepage.h"

SmartPlaylistWizardTypePage::SmartPlaylistWizardTypePage() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  name_ = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name_), "Playlist name");
  dynamic_ = gtk_check_button_new_with_label("Dynamic playlist (refill as songs play)");
  gtk_box_append(GTK_BOX(widget_), gtk_label_new("Type"));
  gtk_box_append(GTK_BOX(widget_), name_);
  gtk_box_append(GTK_BOX(widget_), dynamic_);
}

std::string SmartPlaylistWizardTypePage::name() const {
  const char *text = gtk_editable_get_text(GTK_EDITABLE(name_));
  return text && *text ? text : "Smart playlist";
}

bool SmartPlaylistWizardTypePage::dynamic() const { return gtk_check_button_get_active(GTK_CHECK_BUTTON(dynamic_)); }
