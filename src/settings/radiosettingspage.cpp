#include "settings/radiosettingspage.h"

#include "constants/radiobrowsersettings.h"
#include "constants/radioparadisesettings.h"
#include "constants/somafmsettings.h"
#include "settings/settingspage.h"
#include "settings/streamingsettingslabels.h"
#include "streaming/streamingchoices.h"

AdwPreferencesPage *RadioSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage(RadioSettingsLabels::PageTitle(), "network-wireless-symbolic");

  settings->BeginGroup(SomaFMSettings::kSettingsGroup);
  AdwPreferencesGroup *soma = SettingsPage::AddGroup(page, "SomaFM");
  SettingsPage::AddCombo(soma, settings, SomaFMSettings::kQuality, RadioSettingsLabels::StreamQuality(), StreamingChoices::SomaFmQualities(),
                         SomaFMSettings::kQualityDefault);

  settings->BeginGroup(RadioBrowserSettings::kSettingsGroup);
  AdwPreferencesGroup *browser = SettingsPage::AddGroup(page, "Radio Browser");
  SettingsPage::AddIntEntry(browser, settings, RadioBrowserSettings::kSearchLimit, RadioSettingsLabels::SearchResultsLimit(),
                            RadioBrowserSettings::kSearchLimitDefault);
  SettingsPage::AddToggle(browser, settings, RadioBrowserSettings::kHideBroken, RadioSettingsLabels::HideBroken(), nullptr,
                          RadioBrowserSettings::kHideBrokenDefault);
  SettingsPage::AddEntry(browser, settings, RadioBrowserSettings::kDefaultSort, RadioSettingsLabels::DefaultSortOrder(),
                         RadioBrowserSettings::kDefaultSortDefault);
  SettingsPage::AddEntry(browser, settings, RadioBrowserSettings::kDefaultCountry, RadioSettingsLabels::DefaultCountry());

  settings->BeginGroup(RadioParadiseSettings::kSettingsGroup);
  AdwPreferencesGroup *rp = SettingsPage::AddGroup(page, "Radio Paradise");
  SettingsPage::AddCombo(rp, settings, "quality", "Stream", StreamingChoices::RadioParadiseStreams(), "aac-320");
  return page;
}
