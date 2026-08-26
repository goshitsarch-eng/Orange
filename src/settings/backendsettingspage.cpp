#include "settings/backendsettingspage.h"

#include "constants/backendsettings.h"
#include "core/application.h"
#include "engine/devicefinders.h"
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
  SettingsPage::AddCombo(output, settings, BackendSettings::kALSAPlugin, "ALSA plugin",
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
  SettingsPage::AddIntEntry(buffer, settings, BackendSettings::kBufferDuration, "Buffer duration (ms)",
                            static_cast<int>(BackendSettings::kDefaultBufferDuration));
  SettingsPage::AddEntry(buffer, settings, BackendSettings::kBufferLowWatermark, "Low watermark", "0.33");
  SettingsPage::AddEntry(buffer, settings, BackendSettings::kBufferHighWatermark, "High watermark", "0.99");
  SettingsPage::AddIntEntry(buffer, settings, BackendSettings::kDeviceWarmupDuration, "Device warmup (ms)",
                            BackendSettings::kDefaultDeviceWarmupDuration);

  AdwPreferencesGroup *rg = SettingsPage::AddGroup(page, "ReplayGain / EBU R128");
  SettingsPage::AddToggle(rg, settings, BackendSettings::kRgEnabled, "Enable ReplayGain", nullptr, BackendSettings::kDefaultRgEnabled);
  SettingsPage::AddIntCombo(rg, settings, BackendSettings::kSettingsGroup, BackendSettings::kRgMode, "ReplayGain mode",
                            {{"0", "Album"}, {"1", "Track"}}, BackendSettings::kDefaultRgMode);
  SettingsPage::AddEntry(rg, settings, BackendSettings::kRgPreamp, "ReplayGain preamp (dB)", "0");
  SettingsPage::AddToggle(rg, settings, BackendSettings::kRgCompression, "Prevent clipping", nullptr, BackendSettings::kDefaultRgCompression);
  SettingsPage::AddEntry(rg, settings, BackendSettings::kRgFallbackGain, "Fallback gain (dB)", "0");
  SettingsPage::AddToggle(rg, settings, BackendSettings::kEBUR128LoudnessNormalization, "EBU R128 loudness normalization", nullptr,
                          BackendSettings::kDefaultEBUR128LoudnessNormalization);
  SettingsPage::AddEntry(rg, settings, BackendSettings::kEBUR128TargetLevelLUFS, "EBU R128 target (LUFS)", "-23");

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
  SettingsPage::AddIntEntry(fade, settings, BackendSettings::kFadeoutDuration, "Fade duration (ms)",
                            static_cast<int>(BackendSettings::kDefaultFadeoutDuration));
  SettingsPage::AddIntEntry(fade, settings, BackendSettings::kFadeoutPauseDuration, "Pause fade duration (ms)",
                            static_cast<int>(BackendSettings::kDefaultFadeoutPauseDuration));
  return page;
}
