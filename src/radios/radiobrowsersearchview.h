#ifndef STRAWBERRY_RADIOBROWSERSEARCHVIEW_H
#define STRAWBERRY_RADIOBROWSERSEARCHVIEW_H

#include "radios/radiobrowsersearchmodel.h"

#include <functional>
#include <string>

#include <gtk/gtk.h>

class RadioBrowserSearchView {
 public:
  RadioBrowserSearchView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *entry() const { return entry_; }
  RadioBrowserSearchModel *model() { return &model_; }
  void SetResults(const std::vector<RadioChannel> &results);
  void SetChangedCallback(std::function<void(const std::string &)> callback) { changed_ = std::move(callback); }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *entry_ = nullptr;
  RadioBrowserSearchModel model_;
  std::function<void(const std::string &)> changed_;
};

#endif
