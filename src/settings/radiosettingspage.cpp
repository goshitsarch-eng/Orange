#include "settings/radiosettingspage.h"

#include "constants/radiobrowsersettings.h"
#include "constants/radioparadisesettings.h"
#include "constants/somafmsettings.h"
#include "settings/settingspage.h"
#include "streaming/streamingchoices.h"

AdwPreferencesPage *RadioSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(RadioBrowserSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Radio", "network-wireless-symbolic");
  AdwPreferencesGroup *browser = SettingsPage::AddGroup(page, "Radio Browser");
  SettingsPage::AddEntry(browser, settings, "server", "Radio Browser server", "https://de1.api.radio-browser.info");
  SettingsPage::AddEntry(browser, settings, RadioBrowserSettings::kDefaultCountry, "Country filter");
  SettingsPage::AddToggle(browser, settings, RadioBrowserSettings::kHideBroken, "Hide broken stations", nullptr,
                          RadioBrowserSettings::kHideBrokenDefault);
  SettingsPage::AddIntEntry(browser, settings, RadioBrowserSettings::kSearchLimit, "Search limit", RadioBrowserSettings::kSearchLimitDefault);
  SettingsPage::AddEntry(browser, settings, RadioBrowserSettings::kDefaultSort, "Default sort", RadioBrowserSettings::kDefaultSortDefault);

  settings->BeginGroup(SomaFMSettings::kSettingsGroup);
  AdwPreferencesGroup *soma = SettingsPage::AddGroup(page, "SomaFM");
  SettingsPage::AddCombo(soma, settings, SomaFMSettings::kQuality, "Quality", StreamingChoices::SomaFmQualities(),
                         SomaFMSettings::kQualityDefault);

  settings->BeginGroup(RadioParadiseSettings::kSettingsGroup);
  AdwPreferencesGroup *rp = SettingsPage::AddGroup(page, "Radio Paradise");
  SettingsPage::AddCombo(rp, settings, "quality", "Stream", StreamingChoices::RadioParadiseStreams(), "aac-320");
  return page;
}
