#include "settings/settingspage.h"

#include "core/application.h"
#include "translations/translations.h"
#include "ui/dialogs.h"
#include "utilities/colorutils.h"
#include "utilities/fontutils.h"
#include "widgets/loginstatewidget.h"

#include <cmath>

#include <memory>
#include <string>

namespace SettingsPage {

AdwPreferencesPage *MakePage(const char *name, const char *icon) {
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
  adw_preferences_page_set_title(page, Translations::CStr(name));
  adw_preferences_page_set_icon_name(page, icon);
  if (name && name[0] != '\0') {
    adw_preferences_page_set_name(page, name);
  }
  return page;
}

AdwPreferencesGroup *AddGroup(AdwPreferencesPage *page, const char *title) {
  AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
  if (title && title[0]) {
    adw_preferences_group_set_title(group, Translations::CStr(title));
  }
  adw_preferences_page_add(page, group);
  return group;
}

GtkWidget *AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback,
                     const char *group_name) {
  AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  if (subtitle) {
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), Translations::CStr(subtitle));
  }
  adw_switch_row_set_active(row, settings->BoolValue(key, fallback));
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  if (group_name) {
    g_object_set_data_full(G_OBJECT(row), "settings-group", g_strdup(group_name), g_free);
  }
  g_signal_connect(row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *switch_row, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(switch_row), "settings-key"));
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(switch_row), "settings-group"));
                     if (settings_group) {
                       s->BeginGroup(settings_group);
                     }
                     s->SetBoolValue(settings_key, adw_switch_row_get_active(switch_row));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return GTK_WIDGET(row);
}

GtkWidget *AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback) {
  AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  gtk_editable_set_text(GTK_EDITABLE(row), settings->Value(key, fallback ? fallback : "").c_str());
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->SetValue(settings_key, gtk_editable_get_text(GTK_EDITABLE(entry)));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return GTK_WIDGET(row);
}

GtkWidget *AddPasswordEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback) {
  AdwPasswordEntryRow *row = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  gtk_editable_set_text(GTK_EDITABLE(row), settings->Value(key, fallback ? fallback : "").c_str());
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwPasswordEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->SetValue(settings_key, gtk_editable_get_text(GTK_EDITABLE(entry)));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return GTK_WIDGET(row);
}

void AddDescription(AdwPreferencesGroup *group, const char *text, bool markup) {
  GtkWidget *label = gtk_label_new(nullptr);
  if (markup) {
    gtk_label_set_markup(GTK_LABEL(label), Translations::CStr(text));
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
  } else {
    gtk_label_set_text(GTK_LABEL(label), Translations::CStr(text));
  }
  gtk_label_set_wrap(GTK_LABEL(label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  gtk_widget_add_css_class(label, "dim-label");
  adw_preferences_group_add(group, label);
}

GtkWidget *AddIntEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, int fallback) {
  AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  gtk_editable_set_text(GTK_EDITABLE(row), std::to_string(settings->IntValue(key, fallback)).c_str());
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->SetIntValue(settings_key, g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(entry)), nullptr, 10));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return GTK_WIDGET(row);
}

GtkWidget *AddCombo(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title,
                    const std::vector<std::pair<std::string, std::string>> &choices, const std::string &fallback,
                    const std::function<void(const std::string &)> &changed, const char *group_name) {
  AdwComboRow *row = ADW_COMBO_ROW(adw_combo_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkStringList *model = gtk_string_list_new(nullptr);
  auto *ids = new std::vector<std::string>();
  const std::string current = key && settings ? settings->Value(key, fallback) : fallback;
  guint selected = 0;
  for (size_t i = 0; i < choices.size(); ++i) {
    gtk_string_list_append(model, choices[i].second.c_str());
    ids->push_back(choices[i].first);
    if (choices[i].first == current) {
      selected = static_cast<guint>(i);
    }
  }
  adw_combo_row_set_model(row, G_LIST_MODEL(model));
  adw_combo_row_set_selected(row, selected);
  if (key) {
    g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  }
  if (group_name) {
    g_object_set_data_full(G_OBJECT(row), "settings-group", g_strdup(group_name), g_free);
  }
  g_object_set_data_full(G_OBJECT(row), "choice-ids", ids, [](gpointer p) { delete static_cast<std::vector<std::string> *>(p); });
  if (changed) {
    auto *fn = new std::function<void(const std::string &)>(changed);
    g_object_set_data_full(G_OBJECT(row), "choice-changed", fn, [](gpointer p) { delete static_cast<std::function<void(const std::string &)> *>(p); });
  }
  g_signal_connect(row, "notify::selected", G_CALLBACK(+[](AdwComboRow *combo, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(combo), "settings-key"));
                     auto *choice_ids = static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(combo), "choice-ids"));
                     const guint index = adw_combo_row_get_selected(combo);
                     if (!choice_ids || index >= choice_ids->size()) {
                       return;
                     }
                     const std::string &id = (*choice_ids)[index];
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(combo), "settings-group"));
                     if (s && settings_key) {
                       if (settings_group) {
                         s->BeginGroup(settings_group);
                       }
                       s->SetValue(settings_key, id);
                       s->Sync();
                     }
                     if (auto *fn = static_cast<std::function<void(const std::string &)> *>(g_object_get_data(G_OBJECT(combo), "choice-changed"))) {
                       (*fn)(id);
                     }
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return GTK_WIDGET(row);
}

GtkWidget *AddIntCombo(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                       const std::vector<std::pair<std::string, std::string>> &choices, int fallback) {
  const int current = settings ? settings->IntValue(key, fallback) : fallback;
  return AddCombo(group, settings, key, title, choices, std::to_string(current), [settings, group_name, key](const std::string &id) {
    if (!settings || !key) {
      return;
    }
    if (group_name) {
      settings->BeginGroup(group_name);
    }
    settings->SetIntValue(key, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
    settings->Sync();
  });
}

void AddColorButton(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                    const char *fallback, const char *tooltip) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkColorDialog *dialog = gtk_color_dialog_new();
  GtkWidget *button = gtk_color_dialog_button_new(dialog);
  g_object_unref(dialog);
  if (tooltip && tooltip[0]) {
    gtk_widget_set_tooltip_text(button, Translations::CStr(tooltip));
  }
  const ColorUtils::Rgb rgb = ColorUtils::RgbFromHex(settings ? settings->Value(key, fallback ? fallback : "#ffffff") : (fallback ? fallback : "#ffffff"));
  GdkRGBA rgba;
  rgba.red = rgb.r / 255.0;
  rgba.green = rgb.g / 255.0;
  rgba.blue = rgb.b / 255.0;
  rgba.alpha = 1.0;
  gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(button), &rgba);
  if (group_name) {
    g_object_set_data_full(G_OBJECT(button), "settings-group", g_strdup(group_name), g_free);
  }
  if (key) {
    g_object_set_data_full(G_OBJECT(button), "settings-key", g_strdup(key), g_free);
  }
  g_signal_connect(button, "notify::rgba",
                   G_CALLBACK((+[](GtkColorDialogButton *color, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(color), "settings-group"));
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(color), "settings-key"));
                     if (!s || !settings_key) {
                       return;
                     }
                     const GdkRGBA *value = gtk_color_dialog_button_get_rgba(color);
                     if (!value) {
                       return;
                     }
                     if (settings_group) {
                       s->BeginGroup(settings_group);
                     }
                     const unsigned hex = (static_cast<unsigned>(std::lround(value->red * 255.0)) << 16) |
                                          (static_cast<unsigned>(std::lround(value->green * 255.0)) << 8) |
                                          static_cast<unsigned>(std::lround(value->blue * 255.0));
                     s->SetValue(settings_key, ColorUtils::HexToCss(hex));
                     s->Sync();
                   })),
                   settings);
  adw_action_row_add_suffix(row, button);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddFontButton(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                   const char *fallback) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkFontDialog *dialog = gtk_font_dialog_new();
  GtkWidget *button = gtk_font_dialog_button_new(dialog);
  g_object_unref(dialog);
  const std::string stored = settings ? settings->Value(key, fallback ? fallback : "") : (fallback ? fallback : "");
  const std::string pango = FontUtils::ToPango(FontUtils::Parse(stored));
  PangoFontDescription *desc = pango_font_description_from_string(pango.c_str());
  gtk_font_dialog_button_set_font_desc(GTK_FONT_DIALOG_BUTTON(button), desc);
  pango_font_description_free(desc);
  if (group_name) {
    g_object_set_data_full(G_OBJECT(button), "settings-group", g_strdup(group_name), g_free);
  }
  if (key) {
    g_object_set_data_full(G_OBJECT(button), "settings-key", g_strdup(key), g_free);
  }
  g_signal_connect(button, "notify::font-desc", G_CALLBACK(+[](GtkFontDialogButton *font, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(font), "settings-group"));
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(font), "settings-key"));
                     if (!s || !settings_key) {
                       return;
                     }
                     PangoFontDescription *chosen = gtk_font_dialog_button_get_font_desc(font);
                     if (!chosen) {
                       return;
                     }
                     char *text = pango_font_description_to_string(chosen);
                     if (settings_group) {
                       s->BeginGroup(settings_group);
                     }
                     s->SetValue(settings_key, text ? text : "");
                     s->Sync();
                     g_free(text);
                   }),
                   settings);
  adw_action_row_add_suffix(row, button);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

GtkWidget *AddDoubleScale(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                          double fallback, double min, double max, double step) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request(scale, 180, -1);
  gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
  gtk_range_set_value(GTK_RANGE(scale), settings && key ? settings->DoubleValue(key, fallback) : fallback);
  if (group_name) {
    g_object_set_data_full(G_OBJECT(scale), "settings-group", g_strdup(group_name), g_free);
  }
  if (key) {
    g_object_set_data_full(G_OBJECT(scale), "settings-key", g_strdup(key), g_free);
  }
  g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(range), "settings-group"));
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(range), "settings-key"));
                     if (!s || !settings_key) {
                       return;
                     }
                     if (settings_group) {
                       s->BeginGroup(settings_group);
                     }
                     s->SetDoubleValue(settings_key, gtk_range_get_value(range));
                     s->Sync();
                   }),
                   settings);
  adw_action_row_add_suffix(row, scale);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return scale;
}

GtkWidget *AddIntScale(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                       int fallback, int min, int max, int step) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request(scale, 180, -1);
  gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
  gtk_range_set_value(GTK_RANGE(scale), settings && key ? settings->IntValue(key, fallback) : fallback);
  if (group_name) {
    g_object_set_data_full(G_OBJECT(scale), "settings-group", g_strdup(group_name), g_free);
  }
  if (key) {
    g_object_set_data_full(G_OBJECT(scale), "settings-key", g_strdup(key), g_free);
  }
  g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(range), "settings-group"));
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(range), "settings-key"));
                     if (!s || !settings_key) {
                       return;
                     }
                     if (settings_group) {
                       s->BeginGroup(settings_group);
                     }
                     s->SetIntValue(settings_key, static_cast<int>(gtk_range_get_value(range)));
                     s->Sync();
                   }),
                   settings);
  adw_action_row_add_suffix(row, scale);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  return scale;
}

void AddOpacityScale(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                     double fallback) {
  AddDoubleScale(group, settings, group_name, key, title, fallback, 0.2, 1.0, 0.05);
}

void AddBoolRadios(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *false_title, const char *true_title,
                   bool fallback) {
  const bool value = settings && key ? settings->BoolValue(key, fallback) : fallback;
  GtkWidget *false_btn = gtk_check_button_new_with_label(Translations::CStr(false_title));
  GtkWidget *true_btn = gtk_check_button_new_with_label(Translations::CStr(true_title));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(true_btn), GTK_CHECK_BUTTON(false_btn));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(value ? true_btn : false_btn), TRUE);
  if (key) {
    g_object_set_data_full(G_OBJECT(true_btn), "settings-key", g_strdup(key), g_free);
  }
  g_signal_connect(true_btn, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "settings-key"));
                     if (!s || !settings_key) {
                       return;
                     }
                     s->SetBoolValue(settings_key, gtk_check_button_get_active(button));
                     s->Sync();
                   }),
                   settings);
  AdwActionRow *false_row = ADW_ACTION_ROW(adw_action_row_new());
  adw_action_row_add_prefix(false_row, false_btn);
  adw_action_row_set_activatable_widget(false_row, false_btn);
  AdwActionRow *true_row = ADW_ACTION_ROW(adw_action_row_new());
  adw_action_row_add_prefix(true_row, true_btn);
  adw_action_row_set_activatable_widget(true_row, true_btn);
  adw_preferences_group_add(group, GTK_WIDGET(false_row));
  adw_preferences_group_add(group, GTK_WIDGET(true_row));
}

void AddChoiceRadios(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title,
                     const std::vector<std::pair<std::string, std::string>> &choices, const std::string &fallback,
                     const std::function<void(const std::string &)> &changed) {
  if (title && title[0]) {
    AdwActionRow *header = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(header), Translations::CStr(title));
    adw_preferences_group_add(group, GTK_WIDGET(header));
  }
  const std::string current = key && settings ? settings->Value(key, fallback) : fallback;
  GtkCheckButton *first = nullptr;
  auto *fn = changed ? new std::function<void(const std::string &)>(changed) : nullptr;
  for (const auto &choice : choices) {
    GtkWidget *button = gtk_check_button_new_with_label(Translations::CStr(choice.second.c_str()));
    if (first) {
      gtk_check_button_set_group(GTK_CHECK_BUTTON(button), first);
    } else {
      first = GTK_CHECK_BUTTON(button);
    }
    gtk_check_button_set_active(GTK_CHECK_BUTTON(button), choice.first == current);
    if (key) {
      g_object_set_data_full(G_OBJECT(button), "settings-key", g_strdup(key), g_free);
    }
    g_object_set_data_full(G_OBJECT(button), "choice-id", g_strdup(choice.first.c_str()), g_free);
    if (fn) {
      g_object_set_data(G_OBJECT(button), "choice-changed", fn);
    }
    g_signal_connect(button, "toggled", G_CALLBACK(+[](GtkCheckButton *check, gpointer data) {
                       if (!gtk_check_button_get_active(check)) {
                         return;
                       }
                       auto *s = static_cast<Settings *>(data);
                       const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(check), "settings-key"));
                       const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(check), "choice-id"));
                       if (s && settings_key && id) {
                         s->SetValue(settings_key, id);
                         s->Sync();
                       }
                       if (auto *changed_fn = static_cast<std::function<void(const std::string &)> *>(g_object_get_data(G_OBJECT(check), "choice-changed"))) {
                         (*changed_fn)(id ? id : "");
                       }
                     }),
                     settings);
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_action_row_add_prefix(row, button);
    adw_action_row_set_activatable_widget(row, button);
    adw_preferences_group_add(group, GTK_WIDGET(row));
  }
  if (fn) {
    g_object_set_data_full(G_OBJECT(group), "choice-changed-fn", fn, [](gpointer p) { delete static_cast<std::function<void(const std::string &)> *>(p); });
  }
}

void AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label, const std::function<void()> &clicked,
                  const char *tooltip) {
  AddButtonRow(
      group, title, button_label,
      [clicked](GtkWidget *) {
        if (clicked) {
          clicked();
        }
      },
      tooltip);
}

void AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label,
                  const std::function<void(GtkWidget *button)> &clicked, const char *tooltip) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkWidget *button = gtk_button_new_with_label(Translations::CStr(button_label));
  if (tooltip && tooltip[0]) {
    gtk_widget_set_tooltip_text(button, Translations::CStr(tooltip));
  }
  auto *fn = new std::function<void(GtkWidget *)>(clicked);
  g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                     (*static_cast<std::function<void(GtkWidget *)> *>(data))(GTK_WIDGET(btn));
                   }),
                   fn);
  g_object_set_data_full(G_OBJECT(button), "clicked-fn", fn, +[](gpointer data) { delete static_cast<std::function<void(GtkWidget *)> *>(data); });
  adw_action_row_add_suffix(row, button);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddLoginState(AdwPreferencesGroup *group, Application *app, const char *service_name) {
  if (!app || !service_name) {
    return;
  }
  StreamingService *service = app->streaming_services()->ServiceByName(service_name);
  auto *login = new LoginStateWidget();
  auto alive = std::make_shared<bool>(true);
  login->SetLoggedIn(service && service->logged_in() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut,
                     service_name);
  login->SetLoginCallback([app, service_name]() {
    Dialogs::Login(nullptr, service_name, [app, service_name](const std::string &user, const std::string &token) {
      if (StreamingService *svc = app->streaming_services()->ServiceByName(service_name)) {
        svc->Login(user, token);
      }
    });
  });
  login->SetLogoutCallback([service]() {
    if (service) {
      service->Logout();
    }
  });
  if (service) {
    service->AuthenticationChanged.Connect([login, service, service_name, alive]() {
      if (!*alive || !login || !service) {
        return;
      }
      login->SetLoggedIn(service->logged_in() ? LoginStateWidget::State::LoggedIn : LoginStateWidget::State::LoggedOut, service_name);
    });
    service->AuthenticationFailed.Connect([login, service_name, alive](const std::string &) {
      if (!*alive || !login) {
        return;
      }
      login->SetLoggedIn(LoginStateWidget::State::LoggedOut, service_name);
    });
  }
  adw_preferences_group_add(group, login->widget());
  g_object_set_data_full(G_OBJECT(login->widget()), "login-alive", new std::shared_ptr<bool>(alive),
                         [](gpointer p) { delete static_cast<std::shared_ptr<bool> *>(p); });
  g_signal_connect(login->widget(), "destroy", G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                     if (auto *flag = static_cast<std::shared_ptr<bool> *>(g_object_get_data(G_OBJECT(widget), "login-alive"))) {
                       **flag = false;
                     }
                     delete static_cast<LoginStateWidget *>(data);
                   }),
                   login);
}

}  // namespace SettingsPage
