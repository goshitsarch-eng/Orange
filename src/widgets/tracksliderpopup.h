#ifndef STRAWBERRY_TRACKSLIDERPOPUP_H
#define STRAWBERRY_TRACKSLIDERPOPUP_H

#include <gtk/gtk.h>

#include <string>

class TrackSliderPopup {
 public:
  TrackSliderPopup();
  ~TrackSliderPopup();

  GtkWidget *widget() const { return widget_; }
  void ShowText(GtkWidget *relative, const std::string &text);
  void Hide();

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *label_ = nullptr;
};

#endif
