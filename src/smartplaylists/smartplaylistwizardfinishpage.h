#ifndef STRAWBERRY_SMARTPLAYLISTWIZARDFINISHPAGE_H
#define STRAWBERRY_SMARTPLAYLISTWIZARDFINISHPAGE_H

#include <gtk/gtk.h>

#include <string>

class SmartPlaylistWizardFinishPage {
 public:
  SmartPlaylistWizardFinishPage();

  GtkWidget *widget() const { return widget_; }
  void SetSummary(const std::string &text);

  // Qt SmartPlaylistWizardFinishPage::isComplete: Finish/Create stays off until the name is non-empty.
  static bool IsComplete(const std::string &name) { return !name.empty(); }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *summary_ = nullptr;
};

#endif
