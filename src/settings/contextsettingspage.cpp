#include "settings/contextsettingspage.h"

#include "constants/contextsettings.h"
#include "context/contextformattokens.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

AdwPreferencesPage *ContextSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(ContextSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Context", "view-paged-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Context view");
  SettingsPage::AddEntry(group, settings, ContextSettings::kSettingsTitleFmt, "Title format", ContextSettings::kDefaultTitleFmt);
  SettingsPage::AddEntry(group, settings, ContextSettings::kSettingsSummaryFmt, "Summary format", ContextSettings::kDefaultSummaryFmt);
  AdwPreferencesGroup *tokens = SettingsPage::AddGroup(page, "Insert token");
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(tokens, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(ContextSettings::kSettingsGroup);
      const std::string current = settings->Value(ContextSettings::kSettingsTitleFmt, ContextSettings::kDefaultTitleFmt);
      settings->SetValue(ContextSettings::kSettingsTitleFmt, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }
  SettingsPage::AddToggle(group, settings, ContextSettings::kAlbum, "Show album", nullptr, ContextSettings::kDefaultAlbum);
  SettingsPage::AddToggle(group, settings, ContextSettings::kTechnicalData, "Show technical data", nullptr, ContextSettings::kDefaultTechnicalData);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSongLyrics, "Show lyrics", nullptr, ContextSettings::kDefaultSongLyrics);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSearchCover, "Search for covers", nullptr, true);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSearchLyrics, "Search for lyrics", nullptr, ContextSettings::kDefaultSearchLyrics);
  SettingsPage::AddEntry(group, settings, ContextSettings::kFontHeadline, "Headline font", ContextSettings::kDefaultFontFamily);
  SettingsPage::AddEntry(group, settings, ContextSettings::kFontNormal, "Normal font", ContextSettings::kDefaultFontFamily);
  SettingsPage::AddIntEntry(group, settings, ContextSettings::kFontSizeHeadline, "Headline font size",
                            static_cast<int>(ContextSettings::kDefaultFontSizeHeadline));
  SettingsPage::AddIntEntry(group, settings, ContextSettings::kFontSizeNormal, "Normal font size",
                            static_cast<int>(ContextSettings::kDefaultFontSizeNormal));
  return page;
}
