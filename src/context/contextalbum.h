#ifndef STRAWBERRY_CONTEXTALBUM_H
#define STRAWBERRY_CONTEXTALBUM_H

#include <gtk/gtk.h>

#include <functional>
#include <vector>

class ContextAlbum {
 public:
  using SearchCallback = std::function<void()>;

  ContextAlbum();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *image() const { return image_; }
  void SetImage(const std::vector<unsigned char> &data, int pixel_size = 220);
  void Clear();
  void SetSearchCallback(SearchCallback callback);
  void SearchCoverInProgress();
  bool downloading() const { return downloading_; }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *image_ = nullptr;
  SearchCallback search_;
  bool downloading_ = false;
};

#endif
