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
  void ApplySensitive();

  GtkWidget *widget_ = nullptr;
  GtkWidget *random_ = nullptr;
  GtkWidget *field_radio_ = nullptr;
  GtkWidget *field_drop_ = nullptr;
  GtkWidget *descending_ = nullptr;
  GtkWidget *limit_none_ = nullptr;
  GtkWidget *limit_limit_ = nullptr;
  GtkWidget *limit_ = nullptr;
};

#endif
