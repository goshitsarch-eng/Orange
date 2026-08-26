#ifndef STRAWBERRY_SMARTPLAYLISTWIZARDTYPEPAGE_H
#define STRAWBERRY_SMARTPLAYLISTWIZARDTYPEPAGE_H

#include <gtk/gtk.h>

#include <string>

class SmartPlaylistWizardTypePage {
 public:
  SmartPlaylistWizardTypePage();

  GtkWidget *widget() const { return widget_; }
  std::string name() const;
  bool dynamic() const;
  void SetName(const std::string &name);
  void SetDynamic(bool dynamic);

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *name_ = nullptr;
  GtkWidget *dynamic_ = nullptr;
};

#endif
