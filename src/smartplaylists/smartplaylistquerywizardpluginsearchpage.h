#ifndef STRAWBERRY_SMARTPLAYLISTQUERYWIZARDPLUGINSEARCHPAGE_H
#define STRAWBERRY_SMARTPLAYLISTQUERYWIZARDPLUGINSEARCHPAGE_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

#include <vector>

class SmartPlaylistQueryWizardPluginSearchPage {
 public:
  SmartPlaylistQueryWizardPluginSearchPage();
  GtkWidget *widget() const { return widget_; }
  SmartPlaylistSearch search() const;
  void SetSearch(const SmartPlaylistSearch &search);

  // Qt SmartPlaylistQueryWizardPluginSearchPage::isComplete: All songs, or every term widget is valid.
  static bool IsComplete(SmartPlaylistSearch::SearchType type, const std::vector<bool> &terms_valid) {
    if (type == SmartPlaylistSearch::SearchType::All) {
      return true;
    }
    for (bool valid : terms_valid) {
      if (!valid) {
        return false;
      }
    }
    return true;
  }

 private:
  void ApplyTermsSensitive();

  GtkWidget *widget_ = nullptr;
  GtkWidget *type_drop_ = nullptr;
  GtkWidget *terms_group_ = nullptr;
};

#endif
