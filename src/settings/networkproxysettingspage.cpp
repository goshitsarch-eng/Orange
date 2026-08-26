#include "settings/networkproxysettingspage.h"

#include "constants/networkproxysettings.h"
#include "settings/networkproxylabels.h"
#include "settings/settingspage.h"

AdwPreferencesPage *NetworkProxySettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(NetworkProxySettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage(NetworkProxyLabels::PageTitle(), "network-workgroup-symbolic");
  AdwPreferencesGroup *modes = SettingsPage::AddGroup(page);
  AdwPreferencesGroup *manual = SettingsPage::AddGroup(page);
  const int current_mode = settings->IntValue(NetworkProxySettings::kMode, static_cast<int>(NetworkProxySettings::kDefaultMode));
  gtk_widget_set_sensitive(GTK_WIDGET(manual), NetworkProxyLabels::ManualEnabled(current_mode) ? TRUE : FALSE);
  SettingsPage::AddChoiceRadios(modes, settings, NetworkProxySettings::kMode, nullptr, NetworkProxyLabels::ModeChoices(),
                                std::to_string(current_mode), [settings, manual](const std::string &id) {
                                  if (settings) {
                                    settings->BeginGroup(NetworkProxySettings::kSettingsGroup);
                                    settings->SetIntValue(NetworkProxySettings::kMode, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                                    settings->Sync();
                                  }
                                  gtk_widget_set_sensitive(GTK_WIDGET(manual), NetworkProxyLabels::ManualEnabled(id) ? TRUE : FALSE);
                                });
  SettingsPage::AddIntCombo(manual, settings, NetworkProxySettings::kSettingsGroup, NetworkProxySettings::kType, nullptr,
                            NetworkProxyLabels::TypeChoices(), static_cast<int>(NetworkProxySettings::kDefaultType));
  SettingsPage::AddEntry(manual, settings, NetworkProxySettings::kHostname, "Hostname");
  SettingsPage::AddIntEntry(manual, settings, NetworkProxySettings::kPort, NetworkProxyLabels::Port(),
                            static_cast<int>(NetworkProxySettings::kDefaultPort));
  SettingsPage::AddToggle(manual, settings, NetworkProxySettings::kUseAuthentication, NetworkProxyLabels::AuthTitle(), nullptr,
                          NetworkProxySettings::kDefaultUseAuthentication);
  SettingsPage::AddEntry(manual, settings, NetworkProxySettings::kUsername, NetworkProxyLabels::Username());
  SettingsPage::AddEntry(manual, settings, NetworkProxySettings::kPassword, NetworkProxyLabels::Password());
  SettingsPage::AddToggle(manual, settings, NetworkProxySettings::kEngine, NetworkProxyLabels::EngineLabel(), NetworkProxyLabels::EngineTooltip(),
                          NetworkProxySettings::kDefaultEngine);
  return page;
}
