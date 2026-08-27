#include "smartplaylists/smartplaylistwizardtypepage.h"

#include "smartplaylists/smartplaylistwizardlabels.h"

SmartPlaylistWizardTypePage::SmartPlaylistWizardTypePage() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  name_ = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name_), "Playlist name");
  dynamic_ = gtk_check_button_new_with_label(SmartPlaylistWizardLabels::UseDynamic());
  GtkWidget *hint = gtk_label_new(SmartPlaylistWizardLabels::DynamicHint());
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_widget_set_halign(hint, GTK_ALIGN_START);
  gtk_label_set_xalign(GTK_LABEL(hint), 0);
  gtk_widget_add_css_class(hint, "dim-label");
  gtk_widget_set_margin_start(hint, 24);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new(SmartPlaylistWizardLabels::Name()));
  gtk_box_append(GTK_BOX(widget_), name_);
  gtk_box_append(GTK_BOX(widget_), dynamic_);
  gtk_box_append(GTK_BOX(widget_), hint);
}

std::string SmartPlaylistWizardTypePage::name() const {
  const char *text = gtk_editable_get_text(GTK_EDITABLE(name_));
  return text ? text : "";
}

bool SmartPlaylistWizardTypePage::dynamic() const { return gtk_check_button_get_active(GTK_CHECK_BUTTON(dynamic_)); }

void SmartPlaylistWizardTypePage::SetName(const std::string &name) {
  gtk_editable_set_text(GTK_EDITABLE(name_), name.c_str());
}

void SmartPlaylistWizardTypePage::SetDynamic(bool dynamic) { gtk_check_button_set_active(GTK_CHECK_BUTTON(dynamic_), dynamic); }
