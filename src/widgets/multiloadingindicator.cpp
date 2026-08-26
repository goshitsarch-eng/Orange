#include "multiloadingindicator.h"

#include <sstream>

MultiLoadingIndicator::MultiLoadingIndicator() {
  root_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  g_object_ref_sink(root_);
  spinner_ = gtk_spinner_new();
  label_ = gtk_label_new("");
  gtk_widget_add_css_class(label_, "dim-label");
  gtk_box_append(GTK_BOX(root_), spinner_);
  gtk_box_append(GTK_BOX(root_), label_);
  Refresh();
}

MultiLoadingIndicator::~MultiLoadingIndicator() {
  if (root_) g_object_unref(root_);
}

void MultiLoadingIndicator::SetTasks(const std::vector<std::string> &names) {
  tasks_ = names;
  Refresh();
}

void MultiLoadingIndicator::AddTask(const std::string &name) {
  tasks_.push_back(name);
  Refresh();
}

void MultiLoadingIndicator::RemoveTask(const std::string &name) {
  for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    if (*it == name) {
      tasks_.erase(it);
      break;
    }
  }
  Refresh();
}

void MultiLoadingIndicator::Refresh() {
  if (tasks_.empty()) {
    gtk_spinner_stop(GTK_SPINNER(spinner_));
    gtk_widget_set_visible(root_, FALSE);
    return;
  }
  gtk_widget_set_visible(root_, TRUE);
  gtk_spinner_start(GTK_SPINNER(spinner_));
  std::ostringstream out;
  if (tasks_.size() == 1) {
    out << tasks_.front();
  } else {
    out << tasks_.size() << " tasks: " << tasks_.front();
    if (tasks_.size() > 1) out << " + " << (tasks_.size() - 1) << " more";
  }
  gtk_label_set_text(GTK_LABEL(label_), out.str().c_str());
}
