#include "settings/networkproxysettingspage.h"

#include "constants/networkproxysettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *NetworkProxySettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(NetworkProxySettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Proxy", "network-workgroup-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Network proxy");
  SettingsPage::AddIntEntry(group, settings, NetworkProxySettings::kMode, "Mode (0 system / 1 direct / 2 manual)",
                            static_cast<int>(NetworkProxySettings::kDefaultMode));
  SettingsPage::AddIntEntry(group, settings, NetworkProxySettings::kType, "Type (1 SOCKS5 / 3 HTTP)",
                            static_cast<int>(NetworkProxySettings::kDefaultType));
  SettingsPage::AddEntry(group, settings, NetworkProxySettings::kHostname, "Hostname");
  SettingsPage::AddIntEntry(group, settings, NetworkProxySettings::kPort, "Port", static_cast<int>(NetworkProxySettings::kDefaultPort));
  SettingsPage::AddToggle(group, settings, NetworkProxySettings::kUseAuthentication, "Use authentication", nullptr,
                          NetworkProxySettings::kDefaultUseAuthentication);
  SettingsPage::AddEntry(group, settings, NetworkProxySettings::kUsername, "Username");
  SettingsPage::AddEntry(group, settings, NetworkProxySettings::kPassword, "Password");
  SettingsPage::AddToggle(group, settings, NetworkProxySettings::kEngine, "Apply to the audio engine", nullptr, NetworkProxySettings::kDefaultEngine);
  return page;
}
