#ifndef STRAWBERRY_CONTEXTALBUM_H
#define STRAWBERRY_CONTEXTALBUM_H

#include "utilities/strutils.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class ContextAlbum {
 public:
  using SearchCallback = std::function<void()>;
  using DropCallback = std::function<void(const std::vector<unsigned char> &)>;

  ContextAlbum();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *image() const { return image_; }
  void SetImage(const std::vector<unsigned char> &data, int pixel_size = 220);
  void Clear();
  void SetSearchCallback(SearchCallback callback);
  void SetDropCallback(DropCallback callback);
  void SearchCoverInProgress();
  bool downloading() const { return downloading_; }

  static bool IsImagePath(const std::string &path) {
    const std::string lower = StrUtils::ToLower(path);
    return lower.size() > 4 && (lower.rfind(".jpg") == lower.size() - 4 || lower.rfind(".png") == lower.size() - 4 ||
                                lower.rfind(".jpeg") == lower.size() - 5 || lower.rfind(".webp") == lower.size() - 5 ||
                                lower.rfind(".gif") == lower.size() - 4);
  }

 private:
  gboolean OnDrop(const GValue *value);

  GtkWidget *widget_ = nullptr;
  GtkWidget *image_ = nullptr;
  GtkWidget *spinner_ = nullptr;
  SearchCallback search_;
  DropCallback drop_;
  bool downloading_ = false;
};

#endif
