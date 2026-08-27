#include "smartplaylists/smartplaylistquerywizardpluginsearchpage.h"

#include "smartplaylists/smartplaylistwizardlabels.h"

SmartPlaylistQueryWizardPluginSearchPage::SmartPlaylistQueryWizardPluginSearchPage() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  const char *type_names[] = {SmartPlaylistWizardLabels::And(), SmartPlaylistWizardLabels::Or(), SmartPlaylistWizardLabels::All(),
                              nullptr};
  type_drop_ = gtk_drop_down_new_from_strings(type_names);
  terms_group_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_append(GTK_BOX(terms_group_), gtk_label_new(SmartPlaylistWizardLabels::SearchTerms()));
  gtk_box_append(GTK_BOX(widget_), gtk_label_new(SmartPlaylistWizardLabels::SearchMode()));
  gtk_box_append(GTK_BOX(widget_), type_drop_);
  gtk_box_append(GTK_BOX(widget_), terms_group_);
  g_signal_connect(type_drop_, "notify::selected", G_CALLBACK(+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     static_cast<SmartPlaylistQueryWizardPluginSearchPage *>(data)->ApplyTermsSensitive();
                   }),
                   this);
}

SmartPlaylistSearch SmartPlaylistQueryWizardPluginSearchPage::search() const {
  SmartPlaylistSearch search;
  search.type = SmartPlaylistWizardLabels::TypeFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(type_drop_))));
  return search;
}

void SmartPlaylistQueryWizardPluginSearchPage::SetSearch(const SmartPlaylistSearch &search) {
  gtk_drop_down_set_selected(GTK_DROP_DOWN(type_drop_), static_cast<guint>(SmartPlaylistWizardLabels::TypeIndex(search.type)));
  ApplyTermsSensitive();
}

void SmartPlaylistQueryWizardPluginSearchPage::ApplyTermsSensitive() {
  gtk_widget_set_sensitive(terms_group_, SmartPlaylistWizardLabels::TermsSensitive(search().type));
}
