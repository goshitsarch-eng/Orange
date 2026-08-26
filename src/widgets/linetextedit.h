#ifndef STRAWBERRY_LINETEXTEDIT_H
#define STRAWBERRY_LINETEXTEDIT_H

#include <functional>
#include <gtk/gtk.h>
#include <string>

class LineTextEdit {
 public:
  LineTextEdit();
  GtkWidget *widget() const { return widget_; }
  void SetText(const std::string &text);
  std::string Text() const;
  void SetChangedCallback(std::function<void(const std::string &)> callback);

 private:
  GtkWidget *widget_ = nullptr;
  std::function<void(std::string)> changed_;
};

#endif
