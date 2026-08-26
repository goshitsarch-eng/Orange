#include "smartplaylists/smartplaylistquerywizardpluginsortpage.h"

#include <vector>

SmartPlaylistQueryWizardPluginSortPage::SmartPlaylistQueryWizardPluginSortPage() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  std::vector<const char *> names;
  const auto fields = SmartPlaylistSearch::FieldNames();
  names.reserve(fields.size() + 1);
  for (const std::string &name : fields) {
    names.push_back(name.c_str());
  }
  names.push_back(nullptr);
  field_drop_ = gtk_drop_down_new_from_strings(names.data());
  descending_ = gtk_check_button_new_with_label("Descending");
  gtk_box_append(GTK_BOX(widget_), gtk_label_new("Sort by"));
  gtk_box_append(GTK_BOX(widget_), field_drop_);
  gtk_box_append(GTK_BOX(widget_), descending_);
}

void SmartPlaylistQueryWizardPluginSortPage::ApplyTo(SmartPlaylistSearch *search) const {
  if (!search) {
    return;
  }
  search->sort_field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_drop_))));
  search->sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(descending_));
}

void SmartPlaylistQueryWizardPluginSortPage::SetSearch(const SmartPlaylistSearch &search) {
  gtk_drop_down_set_selected(GTK_DROP_DOWN(field_drop_), static_cast<guint>(search.sort_field));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(descending_), search.sort_descending);
}
