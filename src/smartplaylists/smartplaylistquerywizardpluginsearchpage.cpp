#include "smartplaylists/smartplaylistquerywizardpluginsearchpage.h"

SmartPlaylistQueryWizardPluginSearchPage::SmartPlaylistQueryWizardPluginSearchPage() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  static const char *type_names[] = {"Match all terms", "Match any term", nullptr};
  type_drop_ = gtk_drop_down_new_from_strings(type_names);
  limit_ = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new("Search terms"));
  gtk_box_append(GTK_BOX(widget_), type_drop_);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new("Limit"));
  gtk_box_append(GTK_BOX(widget_), limit_);
}

SmartPlaylistSearch SmartPlaylistQueryWizardPluginSearchPage::search() const {
  SmartPlaylistSearch search;
  search.type = gtk_drop_down_get_selected(GTK_DROP_DOWN(type_drop_)) == 1 ? SmartPlaylistSearch::SearchType::Or
                                                                           : SmartPlaylistSearch::SearchType::And;
  search.limit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(limit_));
  return search;
}

void SmartPlaylistQueryWizardPluginSearchPage::SetSearch(const SmartPlaylistSearch &search) {
  gtk_drop_down_set_selected(GTK_DROP_DOWN(type_drop_), search.type == SmartPlaylistSearch::SearchType::Or ? 1 : 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit_), search.limit);
}
