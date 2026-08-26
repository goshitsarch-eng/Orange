#ifndef STRAWBERRY_COLLECTIONFILTERWIDGET_H
#define STRAWBERRY_COLLECTIONFILTERWIDGET_H

#include "collection/collectionfilteroptions.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>

class CollectionFilterWidget {
 public:
  using ChangedCallback = std::function<void()>;

  CollectionFilterWidget();

  GtkWidget *widget() const { return widget_; }
  CollectionFilterOptions options() const { return options_; }
  bool unrated_only() const { return unrated_only_; }
  void SetChangedCallback(ChangedCallback callback);

 private:
  void EmitChanged();

  GtkWidget *widget_ = nullptr;
  GtkWidget *age_drop_ = nullptr;
  GtkWidget *rating_drop_ = nullptr;
  GtkWidget *mode_drop_ = nullptr;
  CollectionFilterOptions options_;
  bool unrated_only_ = false;
  ChangedCallback changed_;
};

#endif
