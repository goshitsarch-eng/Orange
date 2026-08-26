#include "settings/settingspage.h"

#include "core/application.h"
#include "translations/translations.h"
#include "ui/dialogs.h"
#include "widgets/loginstatewidget.h"

#include <memory>
#include <string>

namespace SettingsPage {

AdwPreferencesPage *MakePage(const char *name, const char *icon) {
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
  adw_preferences_page_set_title(page, Translations::CStr(name));
  adw_preferences_page_set_icon_name(page, icon);
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

void AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback) {
  AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  if (subtitle) {
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), Translations::CStr(subtitle));
  }
  adw_switch_row_set_active(row, settings->BoolValue(key, fallback));
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *switch_row, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(switch_row), "settings-key"));
                     s->SetBoolValue(settings_key, adw_switch_row_get_active(switch_row));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback) {
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
}

void AddIntEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, int fallback) {
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
}

void AddCombo(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title,
              const std::vector<std::pair<std::string, std::string>> &choices, const std::string &fallback,
              const std::function<void(const std::string &)> &changed) {
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
                     if (s && settings_key) {
                       s->SetValue(settings_key, id);
                       s->Sync();
                     }
                     if (auto *fn = static_cast<std::function<void(const std::string &)> *>(g_object_get_data(G_OBJECT(combo), "choice-changed"))) {
                       (*fn)(id);
                     }
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddIntCombo(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                 const std::vector<std::pair<std::string, std::string>> &choices, int fallback) {
  const int current = settings ? settings->IntValue(key, fallback) : fallback;
  AddCombo(group, settings, key, title, choices, std::to_string(current), [settings, group_name, key](const std::string &id) {
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

void AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label, const std::function<void()> &clicked) {
  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(title));
  GtkWidget *button = gtk_button_new_with_label(Translations::CStr(button_label));
  auto *fn = new std::function<void()>(clicked);
  g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     (*static_cast<std::function<void()> *>(data))();
                   }),
                   fn);
  g_object_set_data_full(G_OBJECT(button), "clicked-fn", fn, +[](gpointer data) { delete static_cast<std::function<void()> *>(data); });
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
