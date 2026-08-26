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
  void ShowAt(GtkWidget *relative, int x, const std::string &text, const std::string &small_text);
  void Hide();

 private:
  void Attach(GtkWidget *relative);
  void SetLabels(const std::string &text, const std::string &small_text);

  GtkWidget *widget_ = nullptr;
  GtkWidget *label_ = nullptr;
  GtkWidget *small_label_ = nullptr;
};

#endif
