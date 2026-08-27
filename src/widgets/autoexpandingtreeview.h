#ifndef STRAWBERRY_AUTOEXPANDINGTREEVIEW_H
#define STRAWBERRY_AUTOEXPANDINGTREEVIEW_H

#include <gtk/gtk.h>
#include <string>
#include <vector>

class AutoExpandingTreeView {
 public:
  AutoExpandingTreeView();
  GtkWidget *widget() const { return widget_; }
  GtkListBox *view() const { return GTK_LIST_BOX(list_); }
  void ExpandAll();
  void AppendRow(const std::string &text);
  void Clear();

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
};

#endif
