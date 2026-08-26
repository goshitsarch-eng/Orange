#include "settings/transcodersettingspage.h"

#include "constants/transcodersettings.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

namespace {

void AddFormatFields(AdwPreferencesGroup *group, Settings *settings, Transcoder::Format format) {
  const char *settings_group = TranscoderSettingsPage::GroupName(format);
  settings->BeginGroup(settings_group);
  switch (TranscoderSettingsPage::FieldsFor(format)) {
    case TranscoderSettingsPage::FieldKind::Mp3:
      SettingsPage::AddIntCombo(group, settings, settings_group, TranscoderSettings::LameMP3Settings::kTarget, "Encoding target",
                                {{"0", "Quality (VBR)"}, {"1", "Bitrate"}}, 1);
      SettingsPage::AddIntScale(group, settings, settings_group, TranscoderSettings::LameMP3Settings::kQuality, "VBR quality (0–9)", 4, 0, 9, 1);
      SettingsPage::AddIntScale(group, settings, settings_group, TranscoderSettings::LameMP3Settings::kBitrate, "Bitrate (kbps)", 320, 32, 320, 16);
      SettingsPage::AddIntCombo(group, settings, settings_group, TranscoderSettings::LameMP3Settings::kEncodingEngineQuality, "Engine quality",
                                {{"0", "Fast"}, {"1", "Standard"}, {"2", "High"}}, 1);
      SettingsPage::AddToggle(group, settings, TranscoderSettings::LameMP3Settings::kCbr, "Constant bitrate", nullptr, false, settings_group);
      SettingsPage::AddToggle(group, settings, TranscoderSettings::LameMP3Settings::kMono, "Mono", nullptr, false, settings_group);
      break;
    case TranscoderSettingsPage::FieldKind::Bitrate:
      SettingsPage::AddIntScale(group, settings, settings_group, "quality", TranscoderSettingsPage::QualityTitle(format), 5, 0, 10, 1);
      break;
    case TranscoderSettingsPage::FieldKind::Quality:
      SettingsPage::AddIntScale(group, settings, settings_group, "quality", TranscoderSettingsPage::QualityTitle(format), 5, 0,
                                TranscoderSettingsPage::QualityMax(format), 1);
      break;
  }
}

}  // namespace

AdwPreferencesPage *TranscoderSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(TranscoderSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Transcoding", "document-save-symbolic");
  AdwPreferencesGroup *intro = SettingsPage::AddGroup(page, nullptr);
  adw_preferences_group_set_description(intro, Translations::CStr(IntroText()));

  AdwPreferencesGroup *defaults = SettingsPage::AddGroup(page, "Defaults");
  SettingsPage::AddCombo(defaults, settings, TranscoderSettings::kLastOutputFormat, "Default format", DefaultFormatChoices(),
                         TranscoderSettings::kDefaultLastOutputFormat, {}, TranscoderSettings::kSettingsGroup);

  GtkWidget *stack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  for (Transcoder::Format format : TabFormats()) {
    AdwPreferencesGroup *tab = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(tab, Translations::CStr(TabLabel(format)));
    AddFormatFields(tab, settings, format);
    gtk_stack_add_titled(GTK_STACK(stack), GTK_WIDGET(tab), TabId(format), Translations::CStr(TabLabel(format)));
  }
  GtkWidget *switcher = gtk_stack_switcher_new();
  gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
  gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));

  AdwPreferencesGroup *tabs = SettingsPage::AddGroup(page, "Encoder options");
  adw_preferences_group_add(tabs, switcher);
  adw_preferences_group_add(tabs, stack);
  settings->BeginGroup(TranscoderSettings::kSettingsGroup);
  return page;
}
