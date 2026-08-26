#ifndef STRAWBERRY_SMARTPLAYLISTWIZARDFINISHPAGE_H
#define STRAWBERRY_SMARTPLAYLISTWIZARDFINISHPAGE_H

#include <gtk/gtk.h>

#include <string>

class SmartPlaylistWizardFinishPage {
 public:
  SmartPlaylistWizardFinishPage();

  GtkWidget *widget() const { return widget_; }
  void SetSummary(const std::string &text);

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *summary_ = nullptr;
};

#endif
