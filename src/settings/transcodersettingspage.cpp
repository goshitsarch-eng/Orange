#include "settings/transcodersettingspage.h"

#include "constants/transcodersettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *TranscoderSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(TranscoderSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Transcoding", "document-save-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Defaults");
  SettingsPage::AddEntry(group, settings, "format", "Default format", TranscoderSettings::kDefaultLastOutputFormat);
  SettingsPage::AddEntry(group, settings, TranscoderSettings::LameMP3Settings::kTarget, "LAME target", "quality");
  SettingsPage::AddEntry(group, settings, TranscoderSettings::LameMP3Settings::kQuality, "LAME quality", "4");
  SettingsPage::AddEntry(group, settings, TranscoderSettings::LameMP3Settings::kBitrate, "LAME bitrate", "320");
  SettingsPage::AddToggle(group, settings, TranscoderSettings::LameMP3Settings::kCbr, "LAME CBR", nullptr, false);
  SettingsPage::AddToggle(group, settings, TranscoderSettings::LameMP3Settings::kMono, "LAME mono", nullptr, false);
  return page;
}
