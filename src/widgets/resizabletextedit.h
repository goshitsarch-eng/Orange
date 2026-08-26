#ifndef STRAWBERRY_RESIZABLETEXTEDIT_H
#define STRAWBERRY_RESIZABLETEXTEDIT_H

#include <gtk/gtk.h>
#include <string>

class ResizableTextEdit {
 public:
  ResizableTextEdit();
  GtkWidget *widget() const { return widget_; }
  void SetText(const std::string &text);
  std::string Text() const;

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *view_ = nullptr;
};

#endif
