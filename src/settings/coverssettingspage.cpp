#include "settings/coverssettingspage.h"

#include "constants/coverssettings.h"
#include "core/application.h"
#include "covermanager/coverproviders.h"
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
  const std::vector<CoverProvider *> providers = state->app->cover_providers()->All();
  for (size_t i = 0; i < providers.size(); ++i) {
    CoverProvider *provider = providers[i];
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
                         self->app->cover_providers()->Move(index, -1);
                         if (self->settings) {
                           std::vector<std::string> names;
                           for (CoverProvider *item : self->app->cover_providers()->All()) {
                             names.push_back(item->name());
                           }
                           self->settings->BeginGroup(CoversSettings::kSettingsGroup);
                           self->settings->SetValue(CoversSettings::kProviders, LyricsProviderOrder::Join(names));
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
                         self->app->cover_providers()->Move(index, 1);
                         if (self->settings) {
                           std::vector<std::string> names;
                           for (CoverProvider *item : self->app->cover_providers()->All()) {
                             names.push_back(item->name());
                           }
                           self->settings->BeginGroup(CoversSettings::kSettingsGroup);
                           self->settings->SetValue(CoversSettings::kProviders, LyricsProviderOrder::Join(names));
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

AdwPreferencesPage *CoversSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(CoversSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Covers", "image-x-generic-symbolic");
  AdwPreferencesGroup *save = SettingsPage::AddGroup(page, "Saving");
  SettingsPage::AddCombo(save, settings, CoversSettings::kSaveType, "Save destination",
                         {{"1", "Cache"}, {"2", "Album directory"}, {"3", "Embedded"}}, "2");
  SettingsPage::AddCombo(save, settings, CoversSettings::kSaveFilename, "Filename", {{"1", "Hash"}, {"2", "Pattern"}}, "2");
  SettingsPage::AddEntry(save, settings, CoversSettings::kSavePattern, "Cover filename pattern", CoversSettings::kDefaultSavePattern);
  SettingsPage::AddToggle(save, settings, CoversSettings::kSaveOverwrite, "Overwrite existing covers", nullptr, CoversSettings::kDefaultSaveOverwrite);
  SettingsPage::AddToggle(save, settings, CoversSettings::kSaveLowercase, "Lowercase filenames", nullptr, CoversSettings::kDefaultSaveLowercase);
  SettingsPage::AddToggle(save, settings, CoversSettings::kSaveReplaceSpaces, "Replace spaces in filenames", nullptr,
                          CoversSettings::kDefaultSaveReplaceSpaces);
  SettingsPage::AddEntry(save, settings, CoversSettings::kTypes, "Cover types", "art_embedded,art_automatic,art_manual");
  SettingsPage::AddToggle(save, settings, CoversSettings::kAutomaticSearch, "Search for missing covers automatically", nullptr,
                          CoversSettings::kDefaultAutomaticSearch);

  AdwPreferencesGroup *enabled = SettingsPage::AddGroup(page, "Enabled providers");
  if (app) {
    for (CoverProvider *provider : app->cover_providers()->All()) {
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
    g_object_set_data_full(G_OBJECT(page), "cover-provider-state", state, [](gpointer p) { delete static_cast<ProviderListState *>(p); });
    adw_preferences_group_add(order, list);
    RefreshProviderList(state);
  }
  return page;
}
