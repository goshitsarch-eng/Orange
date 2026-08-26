#include "settings/contextsettingspage.h"

#include "constants/contextsettings.h"
#include "context/contextcover.h"
#include "context/contextfont.h"
#include "context/contextformattokens.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "utilities/fontutils.h"

AdwPreferencesPage *ContextSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(ContextSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Context", "view-paged-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Context view");
  SettingsPage::AddEntry(group, settings, ContextSettings::kSettingsTitleFmt, "Title format", ContextSettings::kDefaultTitleFmt);
  SettingsPage::AddEntry(group, settings, ContextSettings::kSettingsSummaryFmt, "Summary format", ContextSettings::kDefaultSummaryFmt);
  AdwPreferencesGroup *title_tokens = SettingsPage::AddGroup(page, "Insert into title");
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(title_tokens, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(ContextSettings::kSettingsGroup);
      const std::string current = settings->Value(ContextSettings::kSettingsTitleFmt, ContextSettings::kDefaultTitleFmt);
      settings->SetValue(ContextSettings::kSettingsTitleFmt, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }
  AdwPreferencesGroup *summary_tokens = SettingsPage::AddGroup(page, "Insert into summary");
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(summary_tokens, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(ContextSettings::kSettingsGroup);
      const std::string current = settings->Value(ContextSettings::kSettingsSummaryFmt, ContextSettings::kDefaultSummaryFmt);
      settings->SetValue(ContextSettings::kSettingsSummaryFmt, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }
  SettingsPage::AddToggle(group, settings, ContextSettings::kAlbum, "Show album", nullptr, ContextSettings::kDefaultAlbum);
  SettingsPage::AddToggle(group, settings, ContextSettings::kTechnicalData, "Show technical data", nullptr, ContextSettings::kDefaultTechnicalData);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSongLyrics, "Show lyrics", nullptr, ContextSettings::kDefaultSongLyrics);
  AdwSwitchRow *search_cover = ADW_SWITCH_ROW(adw_switch_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(search_cover), Translations::CStr("Automatically search for album cover"));
  adw_switch_row_set_active(search_cover, ContextCover::LoadEnabled(*settings));
  g_signal_connect(search_cover, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     ContextCover::PersistEnabled(*s, adw_switch_row_get_active(row));
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(search_cover));
  settings->BeginGroup(ContextSettings::kSettingsGroup);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSearchLyrics, "Search for lyrics", nullptr, ContextSettings::kDefaultSearchLyrics);

  const FontUtils::Font headline =
      ContextFont::Load(settings->Value(ContextSettings::kFontHeadline, ContextSettings::kDefaultFontFamily),
                        static_cast<int>(settings->DoubleValue(ContextSettings::kFontSizeHeadline, ContextSettings::kDefaultFontSizeHeadline)));
  const FontUtils::Font normal =
      ContextFont::Load(settings->Value(ContextSettings::kFontNormal, ContextSettings::kDefaultFontFamily),
                        static_cast<int>(settings->DoubleValue(ContextSettings::kFontSizeNormal, ContextSettings::kDefaultFontSizeNormal)));
  const std::string headline_pango = FontUtils::ToPango(headline);
  const std::string normal_pango = FontUtils::ToPango(normal);
  SettingsPage::AddFontButton(group, settings, ContextSettings::kSettingsGroup, ContextSettings::kFontHeadline, "Headline font",
                              headline_pango.c_str());
  SettingsPage::AddFontButton(group, settings, ContextSettings::kSettingsGroup, ContextSettings::kFontNormal, "Normal font",
                              normal_pango.c_str());
  return page;
}
