#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H

#include "streaming/streamingcollectionview.h"
#include "streaming/streamingprogress.h"

#include <functional>
#include <gtk/gtk.h>

#include <memory>
#include <string>

class StreamingCollectionViewContainer {
 public:
  using AbortCallback = std::function<void()>;

  explicit StreamingCollectionViewContainer(const std::string &title);

  GtkWidget *widget() const { return widget_; }
  GtkWidget *progressbar() const { return progress_; }
  GtkWidget *abort_button() const { return abort_; }
  StreamingCollectionView *view() const { return view_.get(); }
  void ShowProgress(const std::string &status = {});
  void HideProgress();
  void SetProgress(int value, int maximum = StreamingProgress::kDefaultMaximum);
  void SetProgressMaximum(int maximum);
  void SetProgressStatus(const std::string &status);
  void SetAbortCallback(AbortCallback callback);

 private:
  std::unique_ptr<StreamingCollectionView> view_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *progress_ = nullptr;
  GtkWidget *status_ = nullptr;
  GtkWidget *abort_ = nullptr;
  AbortCallback abort_callback_;
  int progress_max_ = StreamingProgress::kDefaultMaximum;
};

#endif
