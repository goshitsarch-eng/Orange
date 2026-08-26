#ifndef FREESPACEBAR_H
#define FREESPACEBAR_H

#include <cstdint>
#include <string>

#include <gtk/gtk.h>

class FreeSpaceBar {
 public:
  explicit FreeSpaceBar();
  ~FreeSpaceBar();

  FreeSpaceBar(const FreeSpaceBar &) = delete;
  FreeSpaceBar &operator=(const FreeSpaceBar &) = delete;

  GtkWidget *widget() const { return root_; }

  void SetBytes(int64_t used, int64_t additional, int64_t total);
  void SetPath(const std::string &path);

  int64_t used() const { return used_; }
  int64_t additional() const { return additional_; }
  int64_t total() const { return total_; }
  double fraction() const;

 private:
  void Refresh();

  GtkWidget *root_ = nullptr;
  GtkWidget *bar_ = nullptr;
  GtkWidget *label_ = nullptr;
  int64_t used_ = 0;
  int64_t additional_ = 0;
  int64_t total_ = 0;
};

#endif  // FREESPACEBAR_H
