#include "collection/collectionfilterwidget.h"

#include "translations/translations.h"

CollectionFilterWidget::CollectionFilterWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(widget_, 8);
  gtk_widget_set_margin_end(widget_, 8);
  gtk_widget_set_margin_top(widget_, 6);
  gtk_widget_set_margin_bottom(widget_, 4);
  static const char *age_labels[] = {"Any age", "Added today", "Added last week", "Added last month", "Added last 3 months",
                                     "Added last year", nullptr};
  static const char *rating_labels[] = {"Any rating", "Unrated", "1★+", "2★+", "3★+", "4★+", "5★", nullptr};
  static const char *mode_labels[] = {"All songs", "Duplicates", "Untagged", nullptr};
  age_drop_ = gtk_drop_down_new_from_strings(age_labels);
  rating_drop_ = gtk_drop_down_new_from_strings(rating_labels);
  mode_drop_ = gtk_drop_down_new_from_strings(mode_labels);
  gtk_widget_set_hexpand(age_drop_, TRUE);
  gtk_widget_set_hexpand(rating_drop_, TRUE);
  gtk_widget_set_hexpand(mode_drop_, TRUE);
  options_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(options_button_), "view-more-symbolic");
  gtk_widget_set_tooltip_text(options_button_, Translations::CStr("Display options"));
  gtk_box_append(GTK_BOX(widget_), age_drop_);
  gtk_box_append(GTK_BOX(widget_), rating_drop_);
  gtk_box_append(GTK_BOX(widget_), mode_drop_);
  gtk_box_append(GTK_BOX(widget_), options_button_);
  auto notify = +[](GtkDropDown *, GParamSpec *, gpointer data) {
    static_cast<CollectionFilterWidget *>(data)->EmitChanged();
  };
  g_signal_connect(age_drop_, "notify::selected", G_CALLBACK(notify), this);
  g_signal_connect(rating_drop_, "notify::selected", G_CALLBACK(notify), this);
  g_signal_connect(mode_drop_, "notify::selected", G_CALLBACK(notify), this);
  grouping_ = CollectionGrouping::LoadCurrent();
  BuildMenu();
}

void CollectionFilterWidget::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void CollectionFilterWidget::SetGrouping(const CollectionGrouping::Grouping &grouping) {
  grouping_ = grouping;
  ReloadMenu();
}

void CollectionFilterWidget::ReloadMenu() { BuildMenu(); }

void CollectionFilterWidget::ApplyPreset(int index) {
  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  if (index < 0 || static_cast<size_t>(index) >= presets.size()) {
    return;
  }
  const CollectionFilterMenu::Preset &preset = presets[static_cast<size_t>(index)];
  if (preset.advanced) {
    if (menu_action_) {
      menu_action_(CollectionFilterMenu::ActionKind::Advanced);
    }
    return;
  }
  grouping_ = preset.grouping;
  if (grouping_changed_) {
    grouping_changed_(grouping_);
  }
}

void CollectionFilterWidget::ApplySaved(int index) {
  const auto saved = CollectionGrouping::LoadSaved();
  if (index < 0 || static_cast<size_t>(index) >= saved.size()) {
    return;
  }
  grouping_ = saved[static_cast<size_t>(index)].second;
  if (grouping_changed_) {
    grouping_changed_(grouping_);
  }
}

void CollectionFilterWidget::BuildMenu() {
  GMenu *root = g_menu_new();
  GMenu *group_by = g_menu_new();
  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  for (size_t i = 0; i < presets.size(); ++i) {
    const CollectionFilterMenu::Preset &preset = presets[i];
    if (preset.advanced) {
      continue;
    }
    char action[64];
    g_snprintf(action, sizeof(action), "collfilter.preset(%d)", static_cast<int>(i));
    g_menu_append(group_by, Translations::CStr(preset.label), action);
  }
  const auto saved = CollectionGrouping::LoadSaved();
  if (!saved.empty()) {
    GMenu *saved_section = g_menu_new();
    for (size_t i = 0; i < saved.size(); ++i) {
      char action[64];
      g_snprintf(action, sizeof(action), "collfilter.saved(%d)", static_cast<int>(i));
      g_menu_append(saved_section, saved[i].first.c_str(), action);
    }
    g_menu_append_section(group_by, nullptr, G_MENU_MODEL(saved_section));
    g_object_unref(saved_section);
  }
  g_menu_append(group_by, Translations::CStr("Advanced grouping…"), "collfilter.advanced");
  g_menu_append_submenu(root, Translations::CStr("Group by"), G_MENU_MODEL(group_by));
  g_menu_append(root, Translations::CStr("Save grouping…"), "collfilter.save");
  g_menu_append(root, Translations::CStr("Manage groupings…"), "collfilter.manage");

  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *preset = g_simple_action_new("preset", G_VARIANT_TYPE_INT32);
  g_signal_connect(preset, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                     static_cast<CollectionFilterWidget *>(data)->ApplyPreset(g_variant_get_int32(param));
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(preset));
  GSimpleAction *saved_action = g_simple_action_new("saved", G_VARIANT_TYPE_INT32);
  g_signal_connect(saved_action, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                     static_cast<CollectionFilterWidget *>(data)->ApplySaved(g_variant_get_int32(param));
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(saved_action));
  auto add_kind = [&](const char *name, CollectionFilterMenu::ActionKind kind) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_object_set_data(G_OBJECT(action), "kind", GINT_TO_POINTER(static_cast<int>(kind) + 1));
    g_signal_connect(action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer data) {
                       auto *self = static_cast<CollectionFilterWidget *>(data);
                       const int kind = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(act), "kind")) - 1;
                       if (self->menu_action_) {
                         self->menu_action_(static_cast<CollectionFilterMenu::ActionKind>(kind));
                       }
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  };
  add_kind("advanced", CollectionFilterMenu::ActionKind::Advanced);
  add_kind("save", CollectionFilterMenu::ActionKind::Save);
  add_kind("manage", CollectionFilterMenu::ActionKind::Manage);
  gtk_widget_insert_action_group(options_button_, "collfilter", G_ACTION_GROUP(group));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(options_button_), G_MENU_MODEL(root));
  g_object_unref(group);
  g_object_unref(root);
  g_object_unref(group_by);
}

void CollectionFilterWidget::EmitChanged() {
  static const int days[] = {-1, 1, 7, 30, 90, 365};
  static const float ratings[] = {-1.0f, -2.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
  const guint age = gtk_drop_down_get_selected(GTK_DROP_DOWN(age_drop_));
  const guint rating = gtk_drop_down_get_selected(GTK_DROP_DOWN(rating_drop_));
  const guint mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(mode_drop_));
  options_.set_max_age(age < G_N_ELEMENTS(days) && days[age] > 0 ? days[age] * 86400 : -1);
  unrated_only_ = rating < G_N_ELEMENTS(ratings) && ratings[rating] <= -2.0f;
  options_.set_min_rating(unrated_only_ ? -1.0f : (rating < G_N_ELEMENTS(ratings) ? ratings[rating] : -1.0f));
  if (mode == 1) {
    options_.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  } else if (mode == 2) {
    options_.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  } else {
    options_.set_filter_mode(CollectionFilterOptions::FilterMode::All);
  }
  if (changed_) {
    changed_();
  }
}
