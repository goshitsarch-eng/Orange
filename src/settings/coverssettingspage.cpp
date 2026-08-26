#include "settings/coverssettingspage.h"

#include "constants/coverssettings.h"
#include "core/application.h"
#include "settings/settingspage.h"

AdwPreferencesPage *CoversSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(CoversSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Covers", "image-x-generic-symbolic");
  AdwPreferencesGroup *save = SettingsPage::AddGroup(page, "Saving");
  SettingsPage::AddIntEntry(save, settings, CoversSettings::kSaveType, "Save destination (0 cache / 1 album dir / 2 embedded)", 1);
  SettingsPage::AddIntEntry(save, settings, CoversSettings::kSaveFilename, "Filename (0 hash / 1 pattern)", 1);
  SettingsPage::AddEntry(save, settings, CoversSettings::kSavePattern, "Cover filename pattern", CoversSettings::kDefaultSavePattern);
  SettingsPage::AddToggle(save, settings, CoversSettings::kSaveOverwrite, "Overwrite existing covers", nullptr, CoversSettings::kDefaultSaveOverwrite);
  SettingsPage::AddToggle(save, settings, CoversSettings::kSaveLowercase, "Lowercase filenames", nullptr, CoversSettings::kDefaultSaveLowercase);
  SettingsPage::AddToggle(save, settings, CoversSettings::kSaveReplaceSpaces, "Replace spaces in filenames", nullptr,
                          CoversSettings::kDefaultSaveReplaceSpaces);
  SettingsPage::AddEntry(save, settings, CoversSettings::kTypes, "Cover types", "art_embedded,art_automatic,art_manual");
  SettingsPage::AddToggle(save, settings, CoversSettings::kAutomaticSearch, "Search for missing covers automatically", nullptr,
                          CoversSettings::kDefaultAutomaticSearch);

  AdwPreferencesGroup *providers = SettingsPage::AddGroup(page, "Providers");
  if (app) {
    for (CoverProvider *provider : app->cover_providers()->All()) {
      SettingsPage::AddToggle(providers, settings, provider->name().c_str(), provider->name().c_str(), nullptr, true);
    }
  }
  return page;
}
