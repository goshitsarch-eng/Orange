#ifndef STRAWBERRY_SMARTPLAYLISTQUERYWIZARDPLUGINSORTPAGE_H
#define STRAWBERRY_SMARTPLAYLISTQUERYWIZARDPLUGINSORTPAGE_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

class SmartPlaylistQueryWizardPluginSortPage {
 public:
  SmartPlaylistQueryWizardPluginSortPage();
  GtkWidget *widget() const { return widget_; }
  void ApplyTo(SmartPlaylistSearch *search) const;
  void SetSearch(const SmartPlaylistSearch &search);

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *field_drop_ = nullptr;
  GtkWidget *descending_ = nullptr;
};

#endif
