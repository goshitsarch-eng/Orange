#ifndef STRAWBERRY_LINEEDIT_H
#define STRAWBERRY_LINEEDIT_H

#include <functional>
#include <gtk/gtk.h>
#include <string>

class LineEdit {
 public:
  LineEdit();
  GtkWidget *widget() const { return widget_; }
  void SetText(const std::string &text);
  std::string Text() const;
  void SetPlaceholder(const std::string &text);
  void SetChangedCallback(std::function<void(const std::string &)> callback);

 private:
  GtkWidget *widget_ = nullptr;
  std::function<void(std::string)> changed_;
};

#endif
