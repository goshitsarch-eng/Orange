#include "multiloadingindicator.h"

MultiLoadingIndicator::MultiLoadingIndicator() {
  root_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  g_object_ref_sink(root_);
  spinner_ = gtk_spinner_new();
  label_ = gtk_label_new("");
  gtk_widget_add_css_class(label_, "dim-label");
  gtk_label_set_ellipsize(GTK_LABEL(label_), PANGO_ELLIPSIZE_END);
  gtk_box_append(GTK_BOX(root_), spinner_);
  gtk_box_append(GTK_BOX(root_), label_);
  Refresh();
}

MultiLoadingIndicator::~MultiLoadingIndicator() {
  if (root_) g_object_unref(root_);
}

void MultiLoadingIndicator::SetTasks(const std::vector<MultiLoadingText::Task> &tasks) {
  tasks_ = tasks;
  Refresh();
}

void MultiLoadingIndicator::SetTasks(const std::vector<std::string> &names) {
  tasks_.clear();
  tasks_.reserve(names.size());
  for (const std::string &name : names) {
    tasks_.push_back({name, 0, 0});
  }
  Refresh();
}

void MultiLoadingIndicator::AddTask(const std::string &name) {
  tasks_.push_back({name, 0, 0});
  Refresh();
}

void MultiLoadingIndicator::RemoveTask(const std::string &name) {
  for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    if (it->name == name) {
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
    gtk_label_set_text(GTK_LABEL(label_), "");
    return;
  }
  gtk_widget_set_visible(root_, TRUE);
  gtk_spinner_start(GTK_SPINNER(spinner_));
  gtk_label_set_text(GTK_LABEL(label_), MultiLoadingText::Format(tasks_).c_str());
}
