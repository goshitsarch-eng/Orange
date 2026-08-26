#ifndef GROUPEDICONVIEW_H
#define GROUPEDICONVIEW_H

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class GroupedIconView {
 public:
  struct Item {
    std::string group;
    std::string title;
    std::string subtitle;
    std::vector<unsigned char> image;
  };

  GroupedIconView();
  ~GroupedIconView();

  GroupedIconView(const GroupedIconView &) = delete;
  GroupedIconView &operator=(const GroupedIconView &) = delete;

  GtkWidget *widget() const { return root_; }

  void SetItems(const std::vector<Item> &items);
  void Clear();
  int item_count() const { return item_count_; }
  void SetActivateCallback(std::function<void(int)> callback) { activate_cb_ = std::move(callback); }

 private:
  GtkWidget *root_ = nullptr;
  GtkWidget *box_ = nullptr;
  int item_count_ = 0;
  std::function<void(int)> activate_cb_;
};

#endif  // GROUPEDICONVIEW_H
