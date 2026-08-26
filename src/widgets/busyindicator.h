#ifndef BUSYINDICATOR_H
#define BUSYINDICATOR_H

#include <gtk/gtk.h>

#include <string>

class BusyIndicator {
 public:
  explicit BusyIndicator();
  ~BusyIndicator();

  BusyIndicator(const BusyIndicator &) = delete;
  BusyIndicator &operator=(const BusyIndicator &) = delete;

  GtkWidget *widget() const { return root_; }

  void Show(const std::string &text = {});
  void Hide();
  bool visible() const { return visible_; }

 private:
  GtkWidget *root_ = nullptr;
  GtkWidget *spinner_ = nullptr;
  GtkWidget *label_ = nullptr;
  bool visible_ = false;
};

#endif  // BUSYINDICATOR_H
