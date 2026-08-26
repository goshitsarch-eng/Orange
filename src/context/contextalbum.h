#ifndef STRAWBERRY_CONTEXTALBUM_H
#define STRAWBERRY_CONTEXTALBUM_H

#include "constants/filefilterconstants.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class ContextAlbum {
 public:
  using SearchCallback = std::function<void()>;
  using DropCallback = std::function<void(const std::vector<unsigned char> &)>;
  using FadeFinishedCallback = std::function<void()>;
  using ActivateCallback = std::function<void()>;

  static constexpr int kFadeTimelineMs = 1000;
  static constexpr int kFadeTickMs = 50;

  ContextAlbum();
  ~ContextAlbum();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *image() const { return image_; }
  void SetImage(const std::vector<unsigned char> &data, int pixel_size = 220);
  void Clear();
  void SetSearchCallback(SearchCallback callback);
  void SetDropCallback(DropCallback callback);
  void SetFadeFinishedCallback(FadeFinishedCallback callback);
  void SetActivateCallback(ActivateCallback callback);
  void SearchCoverInProgress();
  bool downloading() const { return downloading_; }
  bool has_cover() const { return has_cover_; }

  static double FadeInOpacity(int elapsed_ms) {
    if (elapsed_ms <= 0) {
      return 0.0;
    }
    if (elapsed_ms >= kFadeTimelineMs) {
      return 1.0;
    }
    return static_cast<double>(elapsed_ms) / static_cast<double>(kFadeTimelineMs);
  }
  static double FadeOutOpacity(int elapsed_ms) { return 1.0 - FadeInOpacity(elapsed_ms); }

  static bool IsImagePath(const std::string &path) { return FileFilterConstants::PathMatchesGlobs(path, FileFilterConstants::kLoadImages); }

 private:
  gboolean OnDrop(const GValue *value);
  void StartFade(bool to_placeholder);
  void StopFade();
  gboolean FadeTick();
  void SnapshotCurrentToPrevious();
  void ApplyImageData(const std::vector<unsigned char> &data, int pixel_size);

  GtkWidget *widget_ = nullptr;
  GtkWidget *previous_image_ = nullptr;
  GtkWidget *image_ = nullptr;
  GtkWidget *spinner_ = nullptr;
  SearchCallback search_;
  DropCallback drop_;
  FadeFinishedCallback fade_finished_;
  ActivateCallback activate_;
  bool downloading_ = false;
  bool has_cover_ = false;
  bool fading_to_placeholder_ = false;
  int fade_elapsed_ms_ = 0;
  guint fade_timeout_id_ = 0;
};

#endif
