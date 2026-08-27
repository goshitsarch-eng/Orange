#include "settings/analyzersettingspage.h"

#include "analyzer/analyzer.h"
#include "analyzer/analyzerframerate.h"
#include "constants/analyzersettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *AnalyzerSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(AnalyzerSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Analyzer", "utilities-system-monitor-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Analyzer");
  SettingsPage::AddToggle(group, settings, AnalyzerSettings::kEnabled, "Show analyzer", nullptr, AnalyzerSettings::kDefaultEnabled);
  std::vector<std::pair<std::string, std::string>> types;
  for (const std::string &type : Analyzer::Types()) {
    types.emplace_back(type, type);
  }
  SettingsPage::AddCombo(group, settings, AnalyzerSettings::kType, "Style", types, AnalyzerSettings::kDefaultType);
  std::vector<std::pair<std::string, std::string>> rates;
  for (const AnalyzerFramerate::Preset &preset : AnalyzerFramerate::Presets()) {
    rates.emplace_back(std::to_string(preset.fps), preset.label);
  }
  SettingsPage::AddIntCombo(group, settings, AnalyzerSettings::kSettingsGroup, AnalyzerSettings::kFramerate, "Framerate", rates,
                            AnalyzerSettings::kDefaultFramerate);
  return page;
}
