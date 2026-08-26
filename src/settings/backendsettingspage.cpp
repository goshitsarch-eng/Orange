#include "settings/backendsettingspage.h"

#include "constants/backendsettings.h"
#include "core/application.h"
#include "engine/devicefinders.h"
#include "settings/settingscontrols.h"
#include "settings/settingspage.h"

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

  std::vector<std::pair<std::string, std::string>> devices = {{DeviceFinders::ChoiceKey("autoaudiosink", ""), "Default"}};
  if (finders) {
    devices.clear();
    for (const AudioDevice &device : finders->ListDevices()) {
      devices.emplace_back(DeviceFinders::ChoiceKey(device.output, device.id), device.description);
    }
  }
  const std::string current_choice =
      DeviceFinders::ChoiceKey(settings->Value(BackendSettings::kOutput, "autoaudiosink"), settings->Value(BackendSettings::kDevice));
  SettingsPage::AddCombo(output, settings, nullptr, "Device", devices, current_choice, [settings](const std::string &key) {
    std::string sink;
    std::string device;
    DeviceFinders::SplitChoiceKey(key, &sink, &device);
    settings->SetValue(BackendSettings::kOutput, sink);
    settings->SetValue(BackendSettings::kDevice, device);
    settings->Sync();
  });
  SettingsPage::AddChoiceRadios(output, settings, BackendSettings::kALSAPlugin, "ALSA plugin",
                               {{"hw", "hw"}, {"plughw", "plughw"}, {"pcm", "pcm"}}, "hw");
  SettingsPage::AddToggle(output, settings, BackendSettings::kExclusiveMode, "Exclusive mode", nullptr, BackendSettings::kDefaultExclusiveMode);
  SettingsPage::AddToggle(output, settings, BackendSettings::kVolumeControl, "Software volume control", nullptr, BackendSettings::kDefaultVolumeControl);
  SettingsPage::AddToggle(output, settings, BackendSettings::kVolumeExponential, "Exponential volume scale", nullptr,
                          BackendSettings::kDefaultVolumeExponential);
  SettingsPage::AddToggle(output, settings, BackendSettings::kChannelsEnabled, "Force channel count", nullptr, BackendSettings::kDefaultChannelsEnabled);
  SettingsPage::AddIntEntry(output, settings, BackendSettings::kChannels, "Channels", BackendSettings::kDefaultChannels);
  SettingsPage::AddToggle(output, settings, BackendSettings::kBS2B, "Bauer stereo-to-binaural", nullptr, BackendSettings::kDefaultBS2B);
  SettingsPage::AddToggle(output, settings, BackendSettings::kPlaybin3, "Use playbin3", nullptr, BackendSettings::kDefaultPlaybin3);
  SettingsPage::AddToggle(output, settings, BackendSettings::kHTTP2, "HTTP/2", nullptr, BackendSettings::kDefaultHTTP2);
  SettingsPage::AddToggle(output, settings, BackendSettings::kStrictSSL, "Strict SSL", nullptr, BackendSettings::kDefaultStrictSSL);

  AdwPreferencesGroup *buffer = SettingsPage::AddGroup(page, "Buffer");
  const auto buffer_ms = SettingsControls::BufferDurationMs();
  SettingsPage::AddIntScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kBufferDuration, "Buffer duration (ms)",
                           static_cast<int>(BackendSettings::kDefaultBufferDuration), static_cast<int>(buffer_ms.min),
                           static_cast<int>(buffer_ms.max), static_cast<int>(buffer_ms.step));
  const auto watermark = SettingsControls::BufferWatermark();
  SettingsPage::AddDoubleScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kBufferLowWatermark, "Low watermark",
                              BackendSettings::kDefaultBufferLowWatermark, watermark.min, watermark.max, watermark.step);
  SettingsPage::AddDoubleScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kBufferHighWatermark, "High watermark",
                              BackendSettings::kDefaultBufferHighWatermark, watermark.min, watermark.max, watermark.step);
  const auto warmup = SettingsControls::DeviceWarmupMs();
  SettingsPage::AddIntScale(buffer, settings, BackendSettings::kSettingsGroup, BackendSettings::kDeviceWarmupDuration, "Device warmup (ms)",
                           BackendSettings::kDefaultDeviceWarmupDuration, static_cast<int>(warmup.min), static_cast<int>(warmup.max),
                           static_cast<int>(warmup.step));

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
  SettingsPage::AddToggle(fade, settings, BackendSettings::kFadeoutEnabled, "Fade out when stopping", nullptr, BackendSettings::kDefaultFadeoutEnabled);
  SettingsPage::AddToggle(fade, settings, BackendSettings::kCrossfadeEnabled, "Cross-fade on manual track change", nullptr,
                          BackendSettings::kDefaultCrossfadeEnabled);
  SettingsPage::AddToggle(fade, settings, BackendSettings::kAutoCrossfadeEnabled, "Auto cross-fade between tracks", nullptr,
                          BackendSettings::kDefaultAutoCrossfadeEnabled);
  SettingsPage::AddToggle(fade, settings, BackendSettings::kNoCrossfadeSameAlbum, "No cross-fade on the same album", nullptr,
                          BackendSettings::kDefaultNoCrossfadeSameAlbum);
  SettingsPage::AddToggle(fade, settings, BackendSettings::kFadeoutPauseEnabled, "Fade on pause / resume", nullptr,
                          BackendSettings::kDefaultFadeoutPauseEnabled);
  const auto fade_ms = SettingsControls::FadeDurationMs();
  SettingsPage::AddIntScale(fade, settings, BackendSettings::kSettingsGroup, BackendSettings::kFadeoutDuration, "Fade duration (ms)",
                           static_cast<int>(BackendSettings::kDefaultFadeoutDuration), static_cast<int>(fade_ms.min),
                           static_cast<int>(fade_ms.max), static_cast<int>(fade_ms.step));
  const auto pause_ms = SettingsControls::FadePauseDurationMs();
  SettingsPage::AddIntScale(fade, settings, BackendSettings::kSettingsGroup, BackendSettings::kFadeoutPauseDuration, "Pause fade duration (ms)",
                           static_cast<int>(BackendSettings::kDefaultFadeoutPauseDuration), static_cast<int>(pause_ms.min),
                           static_cast<int>(pause_ms.max), static_cast<int>(pause_ms.step));
  return page;
}
