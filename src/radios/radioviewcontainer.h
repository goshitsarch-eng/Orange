#ifndef STRAWBERRY_RADIOVIEWCONTAINER_H
#define STRAWBERRY_RADIOVIEWCONTAINER_H

#include "radios/radiobrowsersearchview.h"
#include "radios/radiomodel.h"
#include "radios/radioview.h"

#include <functional>
#include <memory>
#include <string>

#include <gtk/gtk.h>

class RadioServices;

class RadioViewContainer {
 public:
  explicit RadioViewContainer(RadioServices *services);

  GtkWidget *widget() const { return widget_; }
  RadioView *view() { return view_.get(); }
  RadioBrowserSearchView *search_view() { return search_view_.get(); }
  void Reload();
  void RefreshChannels();
  void Search(const std::string &query);
  void SetActivateCallback(std::function<void(const RadioChannel &)> callback);
  void SetMenuCallback(RadioView::MenuCallback callback);

 private:
  RadioServices *services_ = nullptr;
  GtkWidget *widget_ = nullptr;
  RadioModel model_;
  std::unique_ptr<RadioView> view_;
  std::unique_ptr<RadioBrowserSearchView> search_view_;
};

#endif
