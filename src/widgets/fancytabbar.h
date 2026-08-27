#ifndef STRAWBERRY_FANCYTABBAR_H
#define STRAWBERRY_FANCYTABBAR_H

#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>

class FancyTabBar {
 public:
  FancyTabBar();
  GtkWidget *widget() const { return widget_; }
  void AddTab(const std::string &id, const std::string &title, const std::string &icon);
  void SetActive(const std::string &id);
  const std::string &active() const { return active_; }
  void SetActivateCallback(std::function<void(const std::string &)> callback);

 private:
  GtkWidget *widget_ = nullptr;
  std::string active_;
  std::function<void(std::string)> activate_;
};

#endif
