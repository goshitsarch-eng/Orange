#include "settings/backendsettingspage.h"

#include "constants/backendsettings.h"
#include "core/application.h"
#include "engine/devicefinders.h"
#include "settings/backendoutputchoices.h"
#include "settings/backendsettingslabels.h"
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
  gtk_widget_set_sensitive(state->same, SettingsControls::SameAlbumFadeEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(state->auto_cross)))
                                            ? TRUE
                                            : FALSE);
  gtk_widget_set_sensitive(state->duration, fade_on ? TRUE : FALSE);
  gtk_widget_set_sensitive(state->pause_duration,
                           SettingsControls::PauseFadeEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(state->pause))) ? TRUE : FALSE);
}

struct BackendEnableState {
  Settings *settings = nullptr;
  GtkWidget *alsa_group = nullptr;
  GtkWidget *fade_group = nullptr;
  FadeWidgets *fade = nullptr;
};

void ApplyBackendEnable(BackendEnableState *state) {
  if (!state || !state->settings) {
    return;
  }
  state->settings->BeginGroup(BackendSettings::kSettingsGroup);
  const std::string output = state->settings->Value(BackendSettings::kOutput, "autoaudiosink");
  const std::string device = state->settings->Value(BackendSettings::kDevice);
  if (state->alsa_group) {
    gtk_widget_set_sensitive(state->alsa_group, SettingsControls::AlsaPluginEnabled(output) ? TRUE : FALSE);
  }
  const bool fading = SettingsControls::FadingGroupEnabled(output, device);
  if (state->fade_group) {
    gtk_widget_set_sensitive(state->fade_group, fading ? TRUE : FALSE);
  }
  if (!fading && state->fade) {
    adw_switch_row_set_active(ADW_SWITCH_ROW(state->fade->stop), FALSE);
    adw_switch_row_set_active(ADW_SWITCH_ROW(state->fade->cross), FALSE);
    adw_switch_row_set_active(ADW_SWITCH_ROW(state->fade->auto_cross), FALSE);
  }
  if (state->fade) {
    ApplyFadeSensitivity(state->fade);
  }
}

}  // namespace

AdwPreferencesPage *BackendSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(BackendSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Backend", "audio-card-symbolic");
  auto *enable = new BackendEnableState();
  enable->settings = settings;
  g_object_set_data_full(G_OBJECT(page), "backend-enable", enable, [](gpointer p) { delete static_cast<BackendEnableState *>(p); });
  AdwPreferencesGroup *output = SettingsPage::AddGroup(page, "Output");
  std::vector<std::pair<std::string, std::string>> outputs;
  DeviceFinders *finders = app ? app->device_finders() : nullptr;
  const std::vector<std::string> sink_names = finders ? finders->Outputs() : std::vector<std::string>{"autoaudiosink", "pulsesink", "pipewiresink", "alsasink"};
  for (const std::string &sink : sink_names) {
    outputs.emplace_back(sink, DeviceFinders::OutputLabel(sink));
  }
  SettingsPage::AddCombo(output, settings, BackendSettings::kOutput, "GStreamer output", outputs, "autoaudiosink",
                         [enable](const std::string &) { ApplyBackendEnable(enable); });

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
  SettingsPage::AddCombo(output, settings, nullptr, "Device", devices, current_choice, [settings, custom_entry, enable](const std::string &key) {
    if (BackendOutputChoices::IsCustomKey(key)) {
      if (*custom_entry) {
        gtk_widget_set_sensitive(*custom_entry, TRUE);
      }
      ApplyBackendEnable(enable);
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
    ApplyBackendEnable(enable);
  });
  *custom_entry = SettingsPage::AddEntry(output, settings, BackendSettings::kDevice, BackendOutputChoices::CustomDeviceTitle());
  gtk_widget_set_sensitive(*custom_entry, custom_device ? TRUE : FALSE);
  g_signal_connect(*custom_entry, "changed", G_CALLBACK(+[](AdwEntryRow *, gpointer data) {
                     ApplyBackendEnable(static_cast<BackendEnableState *>(data));
                   }),
                   enable);
  AdwPreferencesGroup *alsa = SettingsPage::AddGroup(page, "ALSA plugin");
  enable->alsa_group = GTK_WIDGET(alsa);
  SettingsPage::AddChoiceRadios(alsa, settings, BackendSettings::kALSAPlugin, "ALSA plugin",
                               {{"hw", "hw"}, {"plughw", "plughw"}, {"pcm", "pcm"}}, "hw");
  SettingsPage::AddToggle(output, settings, BackendSettings::kExclusiveMode, BackendSettingsLabels::Exclusive(), nullptr,
                          BackendSettings::kDefaultExclusiveMode);
  SettingsPage::AddToggle(output, settings, BackendSettings::kVolumeControl, BackendSettingsLabels::VolumeControl(), nullptr,
                          BackendSettings::kDefaultVolumeControl);
  SettingsPage::AddToggle(output, settings, BackendSettings::kVolumeExponential, BackendSettingsLabels::Exponential(),
                          BackendSettingsLabels::ExponentialHint(), BackendSettings::kDefaultVolumeExponential);
  GtkWidget *force_channels = SettingsPage::AddToggle(output, settings, BackendSettings::kChannelsEnabled,
                                                      BackendSettingsLabels::ForceChannels(), nullptr, BackendSettings::kDefaultChannelsEnabled);
  GtkWidget *channels =
      SettingsPage::AddIntEntry(output, settings, BackendSettings::kChannels, BackendSettingsLabels::Channels(), BackendSettings::kDefaultChannels);
  gtk_widget_set_sensitive(channels, SettingsControls::ChannelsSpinEnabled(adw_switch_row_get_active(ADW_SWITCH_ROW(force_channels))) ? TRUE : FALSE);
  g_signal_connect(force_channels, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     gtk_widget_set_sensitive(GTK_WIDGET(data), SettingsControls::ChannelsSpinEnabled(adw_switch_row_get_active(row)) ? TRUE : FALSE);
                   }),
                   channels);
  SettingsPage::AddToggle(output, settings, BackendSettings::kBS2B, BackendSettingsLabels::BS2B(), nullptr, BackendSettings::kDefaultBS2B);
  SettingsPage::AddToggle(output, settings, BackendSettings::kPlaybin3, BackendSettingsLabels::Playbin3(), BackendSettingsLabels::RestartHint(),
                          BackendSettings::kDefaultPlaybin3);
  SettingsPage::AddToggle(output, settings, BackendSettings::kHTTP2, BackendSettingsLabels::HTTP2(), nullptr, BackendSettings::kDefaultHTTP2);
  SettingsPage::AddToggle(output, settings, BackendSettings::kStrictSSL, BackendSettingsLabels::StrictSSL(), nullptr,
                          BackendSettings::kDefaultStrictSSL);

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
                               {{"none", BackendSettingsLabels::NoNormalization()},
                                {"rg", BackendSettingsLabels::ReplayGain()},
                                {"ebu", BackendSettingsLabels::Ebu()}},
                               SettingsControls::NormalizationChoice(settings->BoolValue(BackendSettings::kRgEnabled, BackendSettings::kDefaultRgEnabled),
                                                                    settings->BoolValue(BackendSettings::kEBUR128LoudnessNormalization,
                                                                                        BackendSettings::kDefaultEBUR128LoudnessNormalization)),
                               [settings](const std::string &id) {
                                 settings->BeginGroup(BackendSettings::kSettingsGroup);
                                 settings->SetBoolValue(BackendSettings::kRgEnabled, SettingsControls::NormalizationUsesReplayGain(id));
                                 settings->SetBoolValue(BackendSettings::kEBUR128LoudnessNormalization, SettingsControls::NormalizationUsesEbu(id));
                                 settings->Sync();
                               });
  SettingsPage::AddIntCombo(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgMode, BackendSettingsLabels::ReplayGainMode(),
                            {{"0", BackendSettingsLabels::AlbumMode()}, {"1", BackendSettingsLabels::RadioMode()}},
                            BackendSettings::kDefaultRgMode);
  const auto rg_db = SettingsControls::ReplayGainDb();
  SettingsPage::AddDoubleScale(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgPreamp, BackendSettingsLabels::Preamp(),
                              BackendSettings::kDefaultRgPreamp, rg_db.min, rg_db.max, rg_db.step);
  SettingsPage::AddToggle(rg, settings, BackendSettings::kRgCompression, BackendSettingsLabels::PreventClipping(), nullptr,
                          BackendSettings::kDefaultRgCompression);
  SettingsPage::AddDoubleScale(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgFallbackGain, BackendSettingsLabels::FallbackGain(),
                              BackendSettings::kDefaultRgFallbackGain, rg_db.min, rg_db.max, rg_db.step);
  const auto ebu = SettingsControls::EbuTargetLufs();
  SettingsPage::AddDoubleScale(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kEBUR128TargetLevelLUFS,
                              BackendSettingsLabels::TargetLevel(), BackendSettings::kDefaultEBUR128TargetLevelLUFS, ebu.min, ebu.max, ebu.step);

  AdwPreferencesGroup *fade = SettingsPage::AddGroup(page, "Fading");
  enable->fade_group = GTK_WIDGET(fade);
  GtkWidget *fade_stop =
      SettingsPage::AddToggle(fade, settings, BackendSettings::kFadeoutEnabled, BackendSettingsLabels::FadeStop(), nullptr,
                              BackendSettings::kDefaultFadeoutEnabled);
  GtkWidget *fade_cross = SettingsPage::AddToggle(fade, settings, BackendSettings::kCrossfadeEnabled, BackendSettingsLabels::FadeManual(),
                                                 nullptr, BackendSettings::kDefaultCrossfadeEnabled);
  GtkWidget *fade_auto = SettingsPage::AddToggle(fade, settings, BackendSettings::kAutoCrossfadeEnabled, BackendSettingsLabels::FadeAuto(),
                                                nullptr, BackendSettings::kDefaultAutoCrossfadeEnabled);
  GtkWidget *fade_same = SettingsPage::AddToggle(fade, settings, BackendSettings::kNoCrossfadeSameAlbum, BackendSettingsLabels::FadeSameAlbum(),
                                                nullptr, BackendSettings::kDefaultNoCrossfadeSameAlbum);
  GtkWidget *fade_pause = SettingsPage::AddToggle(fade, settings, BackendSettings::kFadeoutPauseEnabled, BackendSettingsLabels::FadePause(),
                                                 nullptr, BackendSettings::kDefaultFadeoutPauseEnabled);
  const auto fade_ms = SettingsControls::FadeDurationMs();
  GtkWidget *fade_duration =
      SettingsPage::AddIntScale(fade, settings, BackendSettings::kSettingsGroup, BackendSettings::kFadeoutDuration, BackendSettingsLabels::FadeDuration(),
                                static_cast<int>(BackendSettings::kDefaultFadeoutDuration), static_cast<int>(fade_ms.min),
                                static_cast<int>(fade_ms.max), static_cast<int>(fade_ms.step));
  const auto pause_ms = SettingsControls::FadePauseDurationMs();
  GtkWidget *pause_duration = SettingsPage::AddIntScale(
      fade, settings, BackendSettings::kSettingsGroup, BackendSettings::kFadeoutPauseDuration, BackendSettingsLabels::FadeDuration(),
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
  enable->fade = fade_widgets;
  ApplyBackendEnable(enable);
  return page;
}
