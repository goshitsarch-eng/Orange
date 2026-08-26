#ifndef STRAWBERRY_SMARTPLAYLISTQUERYWIZARDPLUGINSEARCHPAGE_H
#define STRAWBERRY_SMARTPLAYLISTQUERYWIZARDPLUGINSEARCHPAGE_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

class SmartPlaylistQueryWizardPluginSearchPage {
 public:
  SmartPlaylistQueryWizardPluginSearchPage();
  GtkWidget *widget() const { return widget_; }
  SmartPlaylistSearch search() const;
  void SetSearch(const SmartPlaylistSearch &search);

 private:
  void ApplyTermsSensitive();

  GtkWidget *widget_ = nullptr;
  GtkWidget *type_drop_ = nullptr;
  GtkWidget *terms_group_ = nullptr;
};

#endif
