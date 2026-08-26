#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H

#include "streaming/streamingcollectionview.h"
#include "streaming/streamingprogress.h"

#include <gtk/gtk.h>

#include <memory>
#include <string>

class StreamingCollectionViewContainer {
 public:
  explicit StreamingCollectionViewContainer(const std::string &title);

  GtkWidget *widget() const { return widget_; }
  GtkWidget *progressbar() const { return progress_; }
  StreamingCollectionView *view() const { return view_.get(); }
  void ShowProgress(const std::string &status = {});
  void HideProgress();
  void SetProgress(int value, int maximum = StreamingProgress::kDefaultMaximum);
  void SetProgressMaximum(int maximum);
  void SetProgressStatus(const std::string &status);

 private:
  std::unique_ptr<StreamingCollectionView> view_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *progress_ = nullptr;
  GtkWidget *status_ = nullptr;
  int progress_max_ = StreamingProgress::kDefaultMaximum;
};

#endif
