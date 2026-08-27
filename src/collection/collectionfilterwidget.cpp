#include "collection/collectionfilterwidget.h"

#include "collection/collectionfilterchoices.h"
#include "collection/collectiongroupingsave.h"
#include "core/appearanceconfigurebuttons.h"
#include "settings/settingspages.h"
#include "translations/translations.h"

#include <adwaita.h>

CollectionFilterWidget::CollectionFilterWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(widget_, 8);
  gtk_widget_set_margin_end(widget_, 8);
  gtk_widget_set_margin_top(widget_, 6);
  gtk_widget_set_margin_bottom(widget_, 4);
  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  options_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(options_button_), "preferences-system-symbolic");
  gtk_widget_set_tooltip_text(options_button_, Translations::CStr("Display options"));
  gtk_box_append(GTK_BOX(widget_), spacer);
  gtk_box_append(GTK_BOX(widget_), options_button_);
  grouping_ = CollectionGrouping::LoadCurrent();
  options_ = CollectionFilterChoices::FromIndices(age_index_, rating_index_, mode_index_);
  BuildMenu();
  ApplyLook();
}

void CollectionFilterWidget::ApplyLook() {
  if (AppearanceConfigureButtons::ShouldApply(AppearanceConfigureButtons::Target::CollectionOptions)) {
    AppearanceConfigureButtons::ApplyWidget(options_button_, AppearanceConfigureButtons::StoredSize());
  }
}

CollectionFilterWidget::~CollectionFilterWidget() {
  if (menu_model_) {
    g_object_unref(menu_model_);
  }
  if (action_group_) {
    g_object_unref(action_group_);
  }
}

void CollectionFilterWidget::AttachActions(GtkWidget *widget) {
  if (!widget || !action_group_) {
    return;
  }
  gtk_widget_insert_action_group(widget, "collfilter", G_ACTION_GROUP(action_group_));
}

void CollectionFilterWidget::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void CollectionFilterWidget::SetGrouping(const CollectionGrouping::Grouping &grouping) {
  grouping_ = grouping;
  ReloadMenu();
}

void CollectionFilterWidget::ReloadMenu() { BuildMenu(); }

void CollectionFilterWidget::SetConfigureLabel(const std::string &label) {
  configure_label_ = label.empty() ? SettingsPages::ConfigureCollectionLabel() : label;
  ReloadMenu();
}

void CollectionFilterWidget::ApplyFilterIndices(int age, int rating, int mode) {
  age_index_ = CollectionFilterChoices::ClampIndex(age, CollectionFilterChoices::kAgeCount);
  rating_index_ = CollectionFilterChoices::ClampIndex(rating, CollectionFilterChoices::kRatingCount);
  mode_index_ = CollectionFilterChoices::ClampIndex(mode, CollectionFilterChoices::kModeCount);
  options_ = CollectionFilterChoices::FromIndices(age_index_, rating_index_, mode_index_);
  if (changed_) {
    changed_();
  }
}

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

void CollectionFilterWidget::PromptSave() {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(CollectionGroupingSave::DialogTitle()),
                                                                Translations::CStr(CollectionGroupingSave::DialogPrompt())));
  GtkWidget *entry = gtk_entry_new();
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "save", Translations::CStr("Save"), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "save", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "save");
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "save") != 0) {
                       return;
                     }
                     auto *self = static_cast<CollectionFilterWidget *>(data);
                     auto *name_entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry"));
                     if (CollectionGroupingSave::Save(gtk_editable_get_text(name_entry), self->grouping_)) {
                       self->ReloadMenu();
                     }
                   }),
                   this);
  GtkWidget *parent = GTK_WIDGET(gtk_widget_get_root(widget_));
  adw_dialog_present(ADW_DIALOG(dialog), parent ? parent : widget_);
}

void CollectionFilterWidget::BuildMenu() {
  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  const auto saved = CollectionGrouping::LoadSaved();
  const int check = CollectionGroupingSave::MenuCheckIndex(grouping_, presets, saved);
  const std::string group_state = CollectionGroupingSave::MenuStateKey(check, static_cast<int>(presets.size()));

  GMenu *root = g_menu_new();
  GMenu *group_by = g_menu_new();
  for (size_t i = 0; i < presets.size(); ++i) {
    const CollectionFilterMenu::Preset &preset = presets[i];
    if (preset.advanced) {
      continue;
    }
    const std::string action = "collfilter.groupby::p" + std::to_string(static_cast<int>(i));
    g_menu_append(group_by, Translations::CStr(preset.label), action.c_str());
  }
  if (!saved.empty()) {
    GMenu *saved_section = g_menu_new();
    for (size_t i = 0; i < saved.size(); ++i) {
      const std::string action = "collfilter.groupby::s" + std::to_string(static_cast<int>(i));
      g_menu_append(saved_section, saved[i].first.c_str(), action.c_str());
    }
    g_menu_append_section(group_by, nullptr, G_MENU_MODEL(saved_section));
    g_object_unref(saved_section);
  }
  g_menu_append(group_by, Translations::CStr("Advanced grouping…"), "collfilter.groupby::advanced");
  g_menu_append_submenu(root, Translations::CStr("Group by"), G_MENU_MODEL(group_by));
  g_menu_append(root, Translations::CStr(CollectionGroupingSave::SaveLabel()), "collfilter.save");
  g_menu_append(root, Translations::CStr(CollectionGroupingSave::ManageLabel()), "collfilter.manage");

  GMenu *age_menu = g_menu_new();
  for (int i = 0; i < CollectionFilterChoices::kAgeCount; ++i) {
    char action[64];
    g_snprintf(action, sizeof(action), "collfilter.age(%d)", i);
    g_menu_append(age_menu, Translations::CStr(CollectionFilterChoices::kAgeLabels[i]), action);
  }
  g_menu_append_submenu(root, Translations::CStr(CollectionFilterChoices::AgeMenuTitle()), G_MENU_MODEL(age_menu));

  GMenu *rating_menu = g_menu_new();
  for (int i = 0; i < CollectionFilterChoices::kRatingCount; ++i) {
    char action[64];
    g_snprintf(action, sizeof(action), "collfilter.rating(%d)", i);
    g_menu_append(rating_menu, Translations::CStr(CollectionFilterChoices::kRatingLabels[i]), action);
  }
  g_menu_append_submenu(root, Translations::CStr(CollectionFilterChoices::RatingMenuTitle()), G_MENU_MODEL(rating_menu));

  GMenu *mode_section = g_menu_new();
  for (int i = 0; i < CollectionFilterChoices::kModeCount; ++i) {
    char action[64];
    g_snprintf(action, sizeof(action), "collfilter.mode(%d)", i);
    g_menu_append(mode_section, Translations::CStr(CollectionFilterChoices::kModeLabels[i]), action);
  }
  g_menu_append_section(root, nullptr, G_MENU_MODEL(mode_section));
  g_menu_append(root, Translations::CStr(configure_label_.c_str()), "collfilter.configure");

  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *groupby = g_simple_action_new_stateful("groupby", G_VARIANT_TYPE_STRING, g_variant_new_string(group_state.c_str()));
  g_signal_connect(groupby, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     auto *self = static_cast<CollectionFilterWidget *>(data);
                     const char *key = g_variant_get_string(param, nullptr);
                     g_simple_action_set_state(action, param);
                     if (g_strcmp0(key, "advanced") == 0) {
                       if (self->menu_action_) {
                         self->menu_action_(CollectionFilterMenu::ActionKind::Advanced);
                       }
                       return;
                     }
                     if (key && key[0] == 'p') {
                       self->ApplyPreset(g_ascii_strtoll(key + 1, nullptr, 10));
                     } else if (key && key[0] == 's') {
                       self->ApplySaved(g_ascii_strtoll(key + 1, nullptr, 10));
                     }
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(groupby));

  GSimpleAction *age = g_simple_action_new_stateful("age", G_VARIANT_TYPE_INT32, g_variant_new_int32(age_index_));
  g_signal_connect(age, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     auto *self = static_cast<CollectionFilterWidget *>(data);
                     g_simple_action_set_state(action, param);
                     self->ApplyFilterIndices(g_variant_get_int32(param), self->rating_index_, self->mode_index_);
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(age));

  GSimpleAction *rating = g_simple_action_new_stateful("rating", G_VARIANT_TYPE_INT32, g_variant_new_int32(rating_index_));
  g_signal_connect(rating, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     auto *self = static_cast<CollectionFilterWidget *>(data);
                     g_simple_action_set_state(action, param);
                     self->ApplyFilterIndices(self->age_index_, g_variant_get_int32(param), self->mode_index_);
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(rating));

  GSimpleAction *mode = g_simple_action_new_stateful("mode", G_VARIANT_TYPE_INT32, g_variant_new_int32(mode_index_));
  g_signal_connect(mode, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     auto *self = static_cast<CollectionFilterWidget *>(data);
                     g_simple_action_set_state(action, param);
                     self->ApplyFilterIndices(self->age_index_, self->rating_index_, g_variant_get_int32(param));
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(mode));

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
  GSimpleAction *save = g_simple_action_new("save", nullptr);
  g_signal_connect(save, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
                     static_cast<CollectionFilterWidget *>(data)->PromptSave();
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(save));
  add_kind("manage", CollectionFilterMenu::ActionKind::Manage);
  add_kind("configure", CollectionFilterMenu::ActionKind::Configure);
  if (action_group_) {
    g_object_unref(action_group_);
  }
  if (menu_model_) {
    g_object_unref(menu_model_);
  }
  action_group_ = group;
  g_object_ref(group);
  menu_model_ = G_MENU_MODEL(root);
  g_object_ref(root);
  gtk_widget_insert_action_group(options_button_, "collfilter", G_ACTION_GROUP(group));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(options_button_), G_MENU_MODEL(root));
  g_object_unref(group);
  g_object_unref(root);
  g_object_unref(group_by);
  g_object_unref(age_menu);
  g_object_unref(rating_menu);
  g_object_unref(mode_section);
}
