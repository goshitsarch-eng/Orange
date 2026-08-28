#include "settings/transcodersettingspage.h"

#include "constants/transcodersettings.h"
#include "settings/settingspage.h"
#include "settings/settingswheelthrough.h"
#include "transcoder/transcoderoptionslabels.h"
#include "translations/translations.h"

#include <string>

namespace {

void AddBitrateScale(AdwPreferencesGroup *group, Settings *settings, Transcoder::Format format) {
  const char *settings_group = TranscoderSettingsPage::GroupName(format);
  const int min = TranscoderSettingsPage::BitrateMinKbps(format);
  const int max = TranscoderSettingsPage::BitrateMaxKbps(format);
  const int fallback = TranscoderSettingsPage::BitrateDefaultKbps(format);
  int stored = fallback * 1000;
  if (settings) {
    settings->BeginGroup(settings_group);
    stored = settings->IntValue("bitrate", stored);
  }
  const int kbps = TranscoderSettingsPage::ClampKbps(format, TranscoderSettingsPage::DisplayKbps(stored));

  AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(TranscoderSettingsPage::BitrateTitle()));
  adw_action_row_set_subtitle(row, Translations::CStr(TranscoderOptionsLabels::Kbps()));
  GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, 1);
  gtk_widget_set_size_request(scale, 180, -1);
  gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
  gtk_range_set_value(GTK_RANGE(scale), kbps);
  g_object_set_data_full(G_OBJECT(scale), "settings-group", g_strdup(settings_group), g_free);
  g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_group = static_cast<const char *>(g_object_get_data(G_OBJECT(range), "settings-group"));
                     if (!s || !settings_group) {
                       return;
                     }
                     s->BeginGroup(settings_group);
                     s->SetIntValue("bitrate", TranscoderSettingsPage::StoreBps(static_cast<int>(gtk_range_get_value(range))));
                     s->Sync();
                   }),
                   settings);
  adw_action_row_add_suffix(row, scale);
  adw_preferences_group_add(group, GTK_WIDGET(row));
  SettingsWheelThrough::Attach(scale);
}

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
      AddBitrateScale(group, settings, format);
      if (format == Transcoder::Format::AAC) {
        SettingsPage::AddIntCombo(group, settings, settings_group, "profile", TranscoderOptionsLabels::Profile(),
                                  {{"1", TranscoderOptionsLabels::Main()},
                                   {"2", TranscoderOptionsLabels::Lc()},
                                   {"3", TranscoderOptionsLabels::Ssr()},
                                   {"4", TranscoderOptionsLabels::Ltp()}},
                                  2);
        SettingsPage::AddToggle(group, settings, "tns", TranscoderOptionsLabels::Tns(), nullptr, false, settings_group);
        SettingsPage::AddToggle(group, settings, "midside", TranscoderOptionsLabels::Midside(), nullptr, true, settings_group);
        SettingsPage::AddIntCombo(group, settings, settings_group, "shortctl", TranscoderOptionsLabels::BlockType(),
                                  {{"0", TranscoderOptionsLabels::NormalBlock()},
                                   {"1", TranscoderOptionsLabels::NoShort()},
                                   {"2", TranscoderOptionsLabels::NoLong()}},
                                  0);
      }
      break;
    case TranscoderSettingsPage::FieldKind::Quality:
      SettingsPage::AddIntScale(group, settings, settings_group, "quality", TranscoderSettingsPage::QualityTitle(format), 5, 0,
                                TranscoderSettingsPage::QualityMax(format), 1);
      if (format == Transcoder::Format::OggVorbis) {
        SettingsPage::AddToggle(group, settings, "managed", TranscoderOptionsLabels::Managed(), nullptr, false, settings_group);
        SettingsPage::AddIntScale(group, settings, settings_group, "bitrate", TranscoderOptionsLabels::TargetBitrate(), -1, -1, 500000, 1000);
        SettingsPage::AddIntScale(group, settings, settings_group, "min-bitrate", TranscoderOptionsLabels::MinBitrate(), -1, -1, 500000, 1000);
        SettingsPage::AddIntScale(group, settings, settings_group, "max-bitrate", TranscoderOptionsLabels::MaxBitrate(), -1, -1, 500000, 1000);
      } else if (format == Transcoder::Format::Speex) {
        SettingsPage::AddIntScale(group, settings, settings_group, "bitrate", TranscoderOptionsLabels::Bitrate(), 0, 0, 256000, 1000);
        SettingsPage::AddIntCombo(group, settings, settings_group, "mode", TranscoderOptionsLabels::EncodingMode(),
                                  {{"0", TranscoderOptionsLabels::Auto()},
                                   {"1", TranscoderOptionsLabels::Uwb()},
                                   {"2", TranscoderOptionsLabels::Wb()},
                                   {"3", TranscoderOptionsLabels::Nb()}},
                                  0);
        SettingsPage::AddToggle(group, settings, "vbr", TranscoderOptionsLabels::Vbr(), nullptr, false, settings_group);
        SettingsPage::AddToggle(group, settings, "vad", TranscoderOptionsLabels::Vad(), nullptr, false, settings_group);
        SettingsPage::AddToggle(group, settings, "dtx", TranscoderOptionsLabels::Dtx(), nullptr, false, settings_group);
        SettingsPage::AddIntScale(group, settings, settings_group, "complexity", TranscoderOptionsLabels::Complexity(), 3, 0, 10, 1);
        SettingsPage::AddIntScale(group, settings, settings_group, "nframes", TranscoderOptionsLabels::Nframes(), 1, 1, 10, 1);
      }
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
