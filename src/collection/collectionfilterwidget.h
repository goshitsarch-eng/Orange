#ifndef STRAWBERRY_COLLECTIONFILTERWIDGET_H
#define STRAWBERRY_COLLECTIONFILTERWIDGET_H

#include "collection/collectionfiltermenu.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectiongrouping.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>

class CollectionFilterWidget {
 public:
  using ChangedCallback = std::function<void()>;
  using GroupingCallback = std::function<void(const CollectionGrouping::Grouping &)>;
  using MenuActionCallback = std::function<void(CollectionFilterMenu::ActionKind)>;

  CollectionFilterWidget();

  GtkWidget *widget() const { return widget_; }
  CollectionFilterOptions options() const { return options_; }
  bool unrated_only() const { return unrated_only_; }
  CollectionGrouping::Grouping grouping() const { return grouping_; }
  void SetChangedCallback(ChangedCallback callback);
  void SetGroupingChangedCallback(GroupingCallback callback) { grouping_changed_ = std::move(callback); }
  void SetMenuActionCallback(MenuActionCallback callback) { menu_action_ = std::move(callback); }
  void SetGrouping(const CollectionGrouping::Grouping &grouping);
  void ReloadMenu();

 private:
  void EmitChanged();
  void BuildMenu();
  void ApplyPreset(int index);
  void ApplySaved(int index);

  GtkWidget *widget_ = nullptr;
  GtkWidget *age_drop_ = nullptr;
  GtkWidget *rating_drop_ = nullptr;
  GtkWidget *mode_drop_ = nullptr;
  GtkWidget *options_button_ = nullptr;
  CollectionFilterOptions options_;
  CollectionGrouping::Grouping grouping_;
  bool unrated_only_ = false;
  ChangedCallback changed_;
  GroupingCallback grouping_changed_;
  MenuActionCallback menu_action_;
};

#endif
