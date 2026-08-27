#ifndef MULTILOADINGINDICATOR_H
#define MULTILOADINGINDICATOR_H

#include "widgets/multiloadingtext.h"

#include <string>
#include <vector>

#include <gtk/gtk.h>

class MultiLoadingIndicator {
 public:
  MultiLoadingIndicator();
  ~MultiLoadingIndicator();

  MultiLoadingIndicator(const MultiLoadingIndicator &) = delete;
  MultiLoadingIndicator &operator=(const MultiLoadingIndicator &) = delete;

  GtkWidget *widget() const { return root_; }

  void SetTasks(const std::vector<MultiLoadingText::Task> &tasks);
  void SetTasks(const std::vector<std::string> &names);
  void AddTask(const std::string &name);
  void RemoveTask(const std::string &name);
  int task_count() const { return static_cast<int>(tasks_.size()); }

 private:
  void Refresh();

  GtkWidget *root_ = nullptr;
  GtkWidget *spinner_ = nullptr;
  GtkWidget *label_ = nullptr;
  std::vector<MultiLoadingText::Task> tasks_;
};

#endif  // MULTILOADINGINDICATOR_H
