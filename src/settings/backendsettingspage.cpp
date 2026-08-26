#include "settings/backendsettingspage.h"

#include "constants/backendsettings.h"
#include "core/application.h"
#include "engine/devicefinders.h"
#include "settings/backendoutputchoices.h"
#include "settings/settingscontrols.h"
#include "settings/settingspage.h"

namespace {

struct FadeWidgets {
  GtkWidget *stop = nullptr;
  GtkWidget *cross = nullptr;
  GtkWidget *auto_cross = nullptr;
  GtkWidget *same = nullptr;
  GtkWidget *pause = nullptr;
  GtkWidget *duration = nullptr;
  GtkWidget *pause_duration = nullptr;
};

void ApplyFadeSensitivity(FadeWidgets *state) {
  const bool fade_on = SettingsControls::FadeDurationEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(state->stop)),
                                                            adw_switch_row_get_active(ADW_SWITCH_ROW(state->cross)),
                                                            adw_switch_row_get_active(ADW_SWITCH_ROW(state->auto_cross)));
  gtk_widget_set_sensitive(state->same, fade_on ? TRUE : FALSE);
  gtk_widget_set_sensitive(state->duration, fade_on ? TRUE : FALSE);
  gtk_widget_set_sensitive(state->pause_duration,
                           SettingsControls::PauseFadeEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(state->pause))) ? TRUE : FALSE);
}

}  // namespace

AdwPreferencesPage *BackendSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(BackendSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Backend", "audio-card-symbolic");
  AdwPreferencesGroup *output = SettingsPage::AddGroup(page, "Output");
  std::vector<std::pair<std::string, std::string>> outputs;
  DeviceFinders *finders = app ? app->device_finders() : nullptr;
  const std::vector<std::string> sink_names = finders ? finders->Outputs() : std::vector<std::string>{"autoaudiosink", "pulsesink", "pipewiresink", "alsasink"};
  for (const std::string &sink : sink_names) {
    outputs.emplace_back(sink, DeviceFinders::OutputLabel(sink));
  }
  SettingsPage::AddCombo(output, settings, BackendSettings::kOutput, "GStreamer output", outputs, "autoaudiosink");

  const std::vector<AudioDevice> listed = finders ? finders->ListDevices() : std::vector<AudioDevice>{};
  std::vector<std::pair<std::string, std::string>> devices = {{DeviceFinders::ChoiceKey("autoaudiosink", ""), "Default"}};
  if (finders) {
    devices.clear();
    for (const AudioDevice &device : listed) {
      devices.emplace_back(DeviceFinders::ChoiceKey(device.output, device.id), device.description);
    }
  }
  BackendOutputChoices::AppendCustom(&devices);
  const std::string current_output = settings->Value(BackendSettings::kOutput, "autoaudiosink");
  const std::string current_device = settings->Value(BackendSettings::kDevice);
  const bool custom_device = BackendOutputChoices::DeviceIsCustom(current_output, current_device, listed);
  const std::string current_choice = BackendOutputChoices::ComboKey(current_output, current_device, listed);
  auto *custom_entry = new GtkWidget *(nullptr);
  g_object_set_data_full(G_OBJECT(page), "custom-device-entry", custom_entry, [](gpointer p) { delete static_cast<GtkWidget **>(p); });
  SettingsPage::AddCombo(output, settings, nullptr, "Device", devices, current_choice, [settings, custom_entry](const std::string &key) {
    if (BackendOutputChoices::IsCustomKey(key)) {
      if (*custom_entry) {
        gtk_widget_set_sensitive(*custom_entry, TRUE);
      }
      return;
    }
    std::string sink;
    std::string device;
    DeviceFinders::SplitChoiceKey(key, &sink, &device);
    settings->SetValue(BackendSettings::kOutput, sink);
    settings->SetValue(BackendSettings::kDevice, device);
    settings->Sync();
    if (*custom_entry) {
      gtk_widget_set_sensitive(*custom_entry, FALSE);
    }
  });
  *custom_entry = SettingsPage::AddEntry(output, settings, BackendSettings::kDevice, BackendOutputChoices::CustomDeviceTitle());
  gtk_widget_set_sensitive(*custom_entry, custom_device ? TRUE : FALSE);
  SettingsPage::AddChoiceRadios(output, settings, BackendSettings::kALSAPlugin, "ALSA plugin",
                               {{"hw", "hw"}, {"plughw", "plughw"}, {"pcm", "pcm"}}, "hw");
  SettingsPage::AddToggle(output, settings, BackendSettings::kExclusiveMode, "Exclusive mode", nullptr, BackendSettings::kDefaultExclusiveMode);
  SettingsPage::AddToggle(output, settings, BackendSettings::kVolumeControl, "Software volume control", nullptr, BackendSettings::kDefaultVolumeControl);
  SettingsPage::AddToggle(output, settings, BackendSettings::kVolumeExponential, "Exponential volume scale", nullptr,
                          BackendSettings::kDefaultVolumeExponential);
  GtkWidget *force_channels =
      SettingsPage::AddToggle(output, settings, BackendSettings::kChannelsEnabled, "Force channel count", nullptr, BackendSettings::kDefaultChannelsEnabled);
  GtkWidget *channels = SettingsPage::AddIntEntry(output, settings, BackendSettings::kChannels, "Channels", BackendSettings::kDefaultChannels);
  gtk_widget_set_sensitive(channels, SettingsControls::ChannelsSpinEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(force_channels))) ? TRUE : FALSE);
  g_signal_connect(force_channels, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     gtk_widget_set_sensitive(GTK_WIDGET(data), SettingsControls::ChannelsSpinEnabled(adw_switch_row_get_active(row)) ? TRUE : FALSE);
                   }),
                   channels);
  SettingsPage::AddToggle(output, settings, BackendSettings::kBS2B, "Bauer stereo-to-binaural", nullptr, BackendSettings::kDefaultBS2B);
  SettingsPage::AddToggle(output, settings, BackendSettings::kPlaybin3, "Use playbin3", nullptr, BackendSettings::kDefaultPlaybin3);
  SettingsPage::AddToggle(output, settings, BackendSettings::kHTTP2, "HTTP/2", nullptr, BackendSettings::kDefaultHTTP2);
  SettingsPage::AddToggle(output, settings, BackendSettings::kStrictSSL, "Strict SSL", nullptr, BackendSettings::kDefaultStrictSSL);

  AdwPreferencesGroup *buffer = SettingsPage::AddGroup(page, "Buffer");
  const auto buffer_ms = SettingsControls::BufferDurationMs();
  GtkWidget *duration_scale =
      SettingsPage::AddIntScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kBufferDuration, "Buffer duration (ms)",
                                static_cast<int>(BackendSettings::kDefaultBufferDuration), static_cast<int>(buffer_ms.min),
                                static_cast<int>(buffer_ms.max), static_cast<int>(buffer_ms.step));
  const auto watermark = SettingsControls::BufferWatermark();
  GtkWidget *low_scale =
      SettingsPage::AddDoubleScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kBufferLowWatermark, "Low watermark",
                                   BackendSettings::kDefaultBufferLowWatermark, watermark.min, watermark.max, watermark.step);
  GtkWidget *high_scale =
      SettingsPage::AddDoubleScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kBufferHighWatermark, "High watermark",
                                   BackendSettings::kDefaultBufferHighWatermark, watermark.min, watermark.max, watermark.step);
  const auto warmup = SettingsControls::DeviceWarmupMs();
  GtkWidget *warmup_scale = SettingsPage::AddIntScale(
      buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kDeviceWarmupDuration, "Device warmup (ms)",
      BackendSettings::kDefaultDeviceWarmupDuration, static_cast<int>(warmup.min), static_cast<int>(warmup.max),
      static_cast<int>(warmup.step));
  SettingsPage::AddButtonRow(buffer, "", "Defaults", [duration_scale, low_scale, high_scale, warmup_scale]() {
    const SettingsControls::BufferValues defaults = SettingsControls::BufferDefaults();
    gtk_range_set_value(GTK_RANGE(duration_scale), defaults.duration_ms);
    gtk_range_set_value(GTK_RANGE(low_scale), defaults.low_watermark);
    gtk_range_set_value(GTK_RANGE(high_scale), defaults.high_watermark);
    gtk_range_set_value(GTK_RANGE(warmup_scale), defaults.warmup_ms);
  });

  AdwPreferencesGroup *rg = SettingsPage::AddGroup(page, "ReplayGain / EBU R128");
  SettingsPage::AddChoiceRadios(rg, settings, nullptr, "Normalization",
                               {{"none", "None"}, {"rg", "ReplayGain"}, {"ebu", "EBU R128"}},
                               SettingsControls::NormalizationChoice(settings->BoolValue(BackendSettings::kRgEnabled, BackendSettings::kDefaultRgEnabled),
                                                                    settings->BoolValue(BackendSettings::kEBUR128LoudnessNormalization,
                                                                                        BackendSettings::kDefaultEBUR128LoudnessNormalization)),
                               [settings](const std::string &id) {
                                 settings->BeginGroup(BackendSettings::kSettingsGroup);
                                 settings->SetBoolValue(BackendSettings::kRgEnabled, SettingsControls::NormalizationUsesReplayGain(id));
                                 settings->SetBoolValue(BackendSettings::kEBUR128LoudnessNormalization, SettingsControls::NormalizationUsesEbu(id));
                                 settings->Sync();
                               });
  SettingsPage::AddIntCombo(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgMode, "ReplayGain mode",
                            {{"0", "Album"}, {"1", "Track"}}, BackendSettings::kDefaultRgMode);
  const auto rg_db = SettingsControls::ReplayGainDb();
  SettingsPage::AddDoubleScale(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgPreamp, "ReplayGain preamp (dB)",
                              BackendSettings::kDefaultRgPreamp, rg_db.min, rg_db.max, rg_db.step);
  SettingsPage::AddToggle(rg, settings, BackendSettings::kRgCompression, "Prevent clipping", nullptr, BackendSettings::kDefaultRgCompression);
  SettingsPage::AddDoubleScale(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgFallbackGain, "Fallback gain (dB)",
                              BackendSettings::kDefaultRgFallbackGain, rg_db.min, rg_db.max, rg_db.step);
  const auto ebu = SettingsControls::EbuTargetLufs();
  SettingsPage::AddDoubleScale(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kEBUR128TargetLevelLUFS, "EBU R128 target (LUFS)",
                              BackendSettings::kDefaultEBUR128TargetLevelLUFS, ebu.min, ebu.max, ebu.step);

  AdwPreferencesGroup *fade = SettingsPage::AddGroup(page, "Fading");
  GtkWidget *fade_stop =
      SettingsPage::AddToggle(fade, settings, BackendSettings::kFadeoutEnabled, "Fade out when stopping", nullptr, BackendSettings::kDefaultFadeoutEnabled);
  GtkWidget *fade_cross = SettingsPage::AddToggle(fade, settings, BackendSettings::kCrossfadeEnabled, "Cross-fade on manual track change",
                                                 nullptr, BackendSettings::kDefaultCrossfadeEnabled);
  GtkWidget *fade_auto = SettingsPage::AddToggle(fade, settings, BackendSettings::kAutoCrossfadeEnabled, "Auto cross-fade between tracks",
                                                nullptr, BackendSettings::kDefaultAutoCrossfadeEnabled);
  GtkWidget *fade_same = SettingsPage::AddToggle(fade, settings, BackendSettings::kNoCrossfadeSameAlbum, "No cross-fade on the same album",
                                                nullptr, BackendSettings::kDefaultNoCrossfadeSameAlbum);
  GtkWidget *fade_pause = SettingsPage::AddToggle(fade, settings, BackendSettings::kFadeoutPauseEnabled, "Fade on pause / resume", nullptr,
                                                 BackendSettings::kDefaultFadeoutPauseEnabled);
  const auto fade_ms = SettingsControls::FadeDurationMs();
  GtkWidget *fade_duration =
      SettingsPage::AddIntScale(fade, settings, BackendSettings::kSettingsGroup, BackendSettings::kFadeoutDuration, "Fade duration (ms)",
                                static_cast<int>(BackendSettings::kDefaultFadeoutDuration), static_cast<int>(fade_ms.min),
                                static_cast<int>(fade_ms.max), static_cast<int>(fade_ms.step));
  const auto pause_ms = SettingsControls::FadePauseDurationMs();
  GtkWidget *pause_duration = SettingsPage::AddIntScale(
      fade, settings, BackendSettings::kSettingsGroup, BackendSettings::kFadeoutPauseDuration, "Pause fade duration (ms)",
      static_cast<int>(BackendSettings::kDefaultFadeoutPauseDuration), static_cast<int>(pause_ms.min), static_cast<int>(pause_ms.max),
      static_cast<int>(pause_ms.step));
  auto *fade_widgets = new FadeWidgets{fade_stop, fade_cross, fade_auto, fade_same, fade_pause, fade_duration, pause_duration};
  ApplyFadeSensitivity(fade_widgets);
  g_object_set_data_full(G_OBJECT(page), "fade-widgets", fade_widgets, [](gpointer p) { delete static_cast<FadeWidgets *>(p); });
  auto connect_fade = [&](GtkWidget *row) {
    g_signal_connect(row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *, GParamSpec *, gpointer data) {
                       ApplyFadeSensitivity(static_cast<FadeWidgets *>(data));
                     }),
                     fade_widgets);
  };
  connect_fade(fade_stop);
  connect_fade(fade_cross);
  connect_fade(fade_auto);
  connect_fade(fade_pause);
  return page;
}
