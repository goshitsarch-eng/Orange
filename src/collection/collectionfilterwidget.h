#ifndef STRAWBERRY_COLLECTIONFILTERWIDGET_H
#define STRAWBERRY_COLLECTIONFILTERWIDGET_H

#include "collection/collectionfiltermenu.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectiongrouping.h"
#include "settings/settingspages.h"

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
  CollectionGrouping::Grouping grouping() const { return grouping_; }
  void SetChangedCallback(ChangedCallback callback);
  void SetGroupingChangedCallback(GroupingCallback callback) { grouping_changed_ = std::move(callback); }
  void SetMenuActionCallback(MenuActionCallback callback) { menu_action_ = std::move(callback); }
  void SetConfigureLabel(const std::string &label);
  void SetGrouping(const CollectionGrouping::Grouping &grouping);
  void ReloadMenu();

 private:
  void ApplyFilterIndices(int age, int rating, int mode);
  void BuildMenu();
  void ApplyPreset(int index);
  void ApplySaved(int index);
  void PromptSave();

  GtkWidget *widget_ = nullptr;
  GtkWidget *options_button_ = nullptr;
  CollectionFilterOptions options_;
  CollectionGrouping::Grouping grouping_;
  int age_index_ = 0;
  int rating_index_ = 0;
  int mode_index_ = 0;
  ChangedCallback changed_;
  GroupingCallback grouping_changed_;
  MenuActionCallback menu_action_;
  std::string configure_label_ = SettingsPages::ConfigureCollectionLabel();
};

#endif
