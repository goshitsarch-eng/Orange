#include "settings/lyricssettingspage.h"

#include "constants/lyricssettings.h"
#include "core/application.h"
#include "lyrics/lyricsproviders.h"
#include "lyrics/lyricsproviderorder.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

namespace {

struct ProviderListState {
  Application *app = nullptr;
  Settings *settings = nullptr;
  GtkWidget *list = nullptr;
};

void RefreshProviderList(ProviderListState *state) {
  if (!state || !state->list || !state->app) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(state->list)) {
    gtk_list_box_remove(GTK_LIST_BOX(state->list), child);
  }
  const std::vector<LyricsProvider *> providers = state->app->lyrics_providers()->All();
  for (size_t i = 0; i < providers.size(); ++i) {
    LyricsProvider *provider = providers[i];
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), provider->name().c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), provider->enabled() ? Translations::CStr("Enabled") : Translations::CStr("Disabled"));
    GtkWidget *up = gtk_button_new_from_icon_name("go-up-symbolic");
    GtkWidget *down = gtk_button_new_from_icon_name("go-down-symbolic");
    gtk_widget_set_sensitive(up, i > 0);
    gtk_widget_set_sensitive(down, i + 1 < providers.size());
    g_object_set_data(G_OBJECT(up), "provider-state", state);
    g_object_set_data(G_OBJECT(down), "provider-state", state);
    g_object_set_data(G_OBJECT(up), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_object_set_data(G_OBJECT(down), "provider-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    g_signal_connect(up, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->lyrics_providers()->Move(index, -1);
                         if (self->settings) {
                           std::vector<std::string> names;
                           for (LyricsProvider *item : self->app->lyrics_providers()->All()) {
                             names.push_back(item->name());
                           }
                           self->settings->BeginGroup(LyricsSettings::kSettingsGroup);
                           self->settings->SetValue(LyricsSettings::kProviders, LyricsProviderOrder::Join(names));
                           self->settings->Sync();
                         }
                         RefreshProviderList(self);
                       }
                     })),
                     nullptr);
    g_signal_connect(down, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *self = static_cast<ProviderListState *>(g_object_get_data(G_OBJECT(button), "provider-state"));
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "provider-index")) - 1;
                       if (self && self->app) {
                         self->app->lyrics_providers()->Move(index, 1);
                         if (self->settings) {
                           std::vector<std::string> names;
                           for (LyricsProvider *item : self->app->lyrics_providers()->All()) {
                             names.push_back(item->name());
                           }
                           self->settings->BeginGroup(LyricsSettings::kSettingsGroup);
                           self->settings->SetValue(LyricsSettings::kProviders, LyricsProviderOrder::Join(names));
                           self->settings->Sync();
                         }
                         RefreshProviderList(self);
                       }
                     })),
                     nullptr);
    adw_action_row_add_suffix(row, up);
    adw_action_row_add_suffix(row, down);
    gtk_list_box_append(GTK_LIST_BOX(state->list), GTK_WIDGET(row));
  }
}

}  // namespace

AdwPreferencesPage *LyricsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(LyricsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Lyrics", "text-x-generic-symbolic");
  AdwPreferencesGroup *enabled = SettingsPage::AddGroup(page, "Enabled providers");
  if (app) {
    for (LyricsProvider *provider : app->lyrics_providers()->All()) {
      SettingsPage::AddToggle(enabled, settings, provider->name().c_str(), provider->name().c_str(), nullptr, provider->enabled());
    }
  }
  AdwPreferencesGroup *order = SettingsPage::AddGroup(page, "Fetch order");
  GtkWidget *hint = gtk_label_new(Translations::CStr("The first enabled provider is tried first."));
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_label_set_xalign(GTK_LABEL(hint), 0);
  gtk_widget_add_css_class(hint, "dim-label");
  adw_preferences_group_add(order, hint);
  if (app) {
    GtkWidget *list = gtk_list_box_new();
    gtk_widget_add_css_class(list, "boxed-list");
    auto *state = new ProviderListState();
    state->app = app;
    state->settings = settings;
    state->list = list;
    g_object_set_data_full(G_OBJECT(page), "provider-state", state, [](gpointer p) { delete static_cast<ProviderListState *>(p); });
    adw_preferences_group_add(order, list);
    RefreshProviderList(state);
  }
  return page;
}
