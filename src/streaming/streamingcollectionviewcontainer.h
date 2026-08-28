#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H

#include "collection/collectionfilterwidget.h"
#include "streaming/streamingcollectionview.h"
#include "streaming/streamingprogress.h"

#include <functional>
#include <gtk/gtk.h>

#include <memory>
#include <string>

class StreamingCollectionViewContainer {
 public:
  using AbortCallback = std::function<void()>;
  using MenuActionCallback = CollectionFilterWidget::MenuActionCallback;

  explicit StreamingCollectionViewContainer(const std::string &title);

  GtkWidget *widget() const { return widget_; }
  GtkWidget *progressbar() const { return progress_; }
  GtkWidget *abort_button() const { return abort_; }
  StreamingCollectionView *view() const { return view_.get(); }
  CollectionFilterWidget *filter_widget() const { return filter_widget_.get(); }
  void ApplyLook();
  void ShowProgress(const std::string &status = {});
  void ShowError(const std::string &status);
  void HideProgress();
  void HideProgressUnlessError();
  void SetProgress(int value, int maximum = StreamingProgress::kDefaultMaximum);
  void SetProgressMaximum(int maximum);
  void SetProgressStatus(const std::string &status);
  void SetAbortCallback(AbortCallback callback);
  void SetMenuActionCallback(MenuActionCallback callback) { menu_action_ = std::move(callback); }
  bool has_error() const { return has_error_; }
  bool working() const { return working_; }

 private:
  void UpdateActionButton();
  void OnActionClicked();

  std::unique_ptr<CollectionFilterWidget> filter_widget_;
  std::unique_ptr<StreamingCollectionView> view_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *progress_ = nullptr;
  GtkWidget *status_ = nullptr;
  GtkWidget *abort_ = nullptr;
  AbortCallback abort_callback_;
  MenuActionCallback menu_action_;
  int progress_max_ = StreamingProgress::kDefaultMaximum;
  bool working_ = false;
  bool has_error_ = false;
};

#endif
