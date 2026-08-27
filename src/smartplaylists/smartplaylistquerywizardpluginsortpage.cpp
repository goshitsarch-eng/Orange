#include "smartplaylists/smartplaylistquerywizardpluginsortpage.h"

#include "smartplaylists/smartplaylistwizardlabels.h"

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
  random_ = gtk_check_button_new_with_label(SmartPlaylistWizardLabels::Random());
  field_radio_ = gtk_check_button_new_with_label(SmartPlaylistWizardLabels::SortBy());
  gtk_check_button_set_group(GTK_CHECK_BUTTON(field_radio_), GTK_CHECK_BUTTON(random_));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(random_), TRUE);
  limit_none_ = gtk_check_button_new_with_label(SmartPlaylistWizardLabels::ShowAll());
  limit_limit_ = gtk_check_button_new_with_label(SmartPlaylistWizardLabels::OnlyFirst());
  gtk_check_button_set_group(GTK_CHECK_BUTTON(limit_limit_), GTK_CHECK_BUTTON(limit_none_));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(limit_none_), TRUE);
  limit_ = gtk_spin_button_new_with_range(1, 1000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit_), 15);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new(SmartPlaylistWizardLabels::Sorting()));
  gtk_box_append(GTK_BOX(widget_), random_);
  gtk_box_append(GTK_BOX(widget_), field_radio_);
  gtk_box_append(GTK_BOX(widget_), field_drop_);
  gtk_box_append(GTK_BOX(widget_), descending_);
  gtk_box_append(GTK_BOX(widget_), gtk_label_new(SmartPlaylistWizardLabels::Limits()));
  gtk_box_append(GTK_BOX(widget_), limit_none_);
  gtk_box_append(GTK_BOX(widget_), limit_limit_);
  GtkWidget *limit_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(limit_row), limit_);
  gtk_box_append(GTK_BOX(limit_row), gtk_label_new(SmartPlaylistWizardLabels::Songs()));
  gtk_box_append(GTK_BOX(widget_), limit_row);
  g_signal_connect(random_, "toggled", G_CALLBACK(+[](GtkCheckButton *, gpointer data) {
                     static_cast<SmartPlaylistQueryWizardPluginSortPage *>(data)->ApplySensitive();
                   }),
                   this);
  g_signal_connect(limit_none_, "toggled", G_CALLBACK(+[](GtkCheckButton *, gpointer data) {
                     static_cast<SmartPlaylistQueryWizardPluginSortPage *>(data)->ApplySensitive();
                   }),
                   this);
  ApplySensitive();
}

void SmartPlaylistQueryWizardPluginSortPage::ApplyTo(SmartPlaylistSearch *search) const {
  if (!search) {
    return;
  }
  search->sort_field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_drop_))));
  search->sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(descending_));
  search->sort_random = gtk_check_button_get_active(GTK_CHECK_BUTTON(random_));
  search->limit = SmartPlaylistWizardLabels::LimitFromUi(gtk_check_button_get_active(GTK_CHECK_BUTTON(limit_none_)),
                                                         gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(limit_)));
}

void SmartPlaylistQueryWizardPluginSortPage::SetSearch(const SmartPlaylistSearch &search) {
  gtk_drop_down_set_selected(GTK_DROP_DOWN(field_drop_), static_cast<guint>(search.sort_field));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(descending_), search.sort_descending);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit_), SmartPlaylistWizardLabels::LimitSpinOrDefault(search.limit));
  if (search.sort_random) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(random_), TRUE);
  } else {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(field_radio_), TRUE);
  }
  if (search.limit > 0) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(limit_limit_), TRUE);
  } else {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(limit_none_), TRUE);
  }
  ApplySensitive();
}

void SmartPlaylistQueryWizardPluginSortPage::ApplySensitive() {
  const bool field = SmartPlaylistWizardLabels::FieldSortSensitive(gtk_check_button_get_active(GTK_CHECK_BUTTON(random_)));
  gtk_widget_set_sensitive(field_drop_, field);
  gtk_widget_set_sensitive(descending_, field);
  gtk_widget_set_sensitive(limit_, SmartPlaylistWizardLabels::LimitSpinSensitive(gtk_check_button_get_active(GTK_CHECK_BUTTON(limit_none_))));
}
