#ifndef STRAWBERRY_RENAMETABLINEEDIT_H
#define STRAWBERRY_RENAMETABLINEEDIT_H

#include <functional>
#include <gtk/gtk.h>
#include <string>

class RenameTabLineEdit {
 public:
  RenameTabLineEdit();
  GtkWidget *widget() const { return widget_; }
  void SetText(const std::string &text);
  std::string Text() const;
  void SetAcceptedCallback(std::function<void(const std::string &)> callback);

 private:
  GtkWidget *widget_ = nullptr;
  std::function<void(std::string)> accepted_;
};

#endif
