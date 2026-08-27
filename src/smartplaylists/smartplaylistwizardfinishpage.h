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

  // Qt Next/Finish also requires the search page to be complete.
  static bool CanCreate(const std::string &name, bool search_complete) { return IsComplete(name) && search_complete; }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *summary_ = nullptr;
};

#endif
