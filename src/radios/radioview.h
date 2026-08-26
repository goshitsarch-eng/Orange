#ifndef STRAWBERRY_RADIOVIEW_H
#define STRAWBERRY_RADIOVIEW_H

#include "radios/radiomodel.h"

#include <functional>

#include <gtk/gtk.h>

class RadioView {
 public:
  RadioView();

  GtkWidget *widget() const { return widget_; }
  void Reload(RadioModel *model);
  void SetActivateCallback(std::function<void(const RadioChannel &)> callback) { activate_ = std::move(callback); }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(const RadioChannel &)> activate_;
};

#endif
