#ifndef STRAWBERRY_ALSAPLUGIN_H
#define STRAWBERRY_ALSAPLUGIN_H

#include <string>

// The ALSA device string chosen in the device list names a card, and the plugin preference decides which ALSA
// interface that card is opened through: "hw" for the raw device, "plughw" to let ALSA convert the format, or
// "pcm" for a named PCM.  The two are stored separately, so the plugin has to be applied to the device string
// before it reaches the sink.
namespace AlsaPlugin {

inline bool IsAlsaOutput(const std::string &output) { return output == "alsasink"; }

inline bool IsKnownPlugin(const std::string &plugin) { return plugin == "hw" || plugin == "plughw" || plugin == "pcm"; }

// "hw:0,0" with plughw becomes "plughw:0,0"; a device with no prefix keeps its own name, since it is a named
// PCM ("default", "front:CARD=PCH") rather than a card reference the plugin can be swapped on.
inline std::string Apply(const std::string &output, const std::string &device, const std::string &plugin) {
  if (!IsAlsaOutput(output) || device.empty() || !IsKnownPlugin(plugin)) {
    return device;
  }
  const std::string::size_type colon = device.find(':');
  if (colon == std::string::npos) {
    return device;
  }
  const std::string prefix = device.substr(0, colon);
  if (!IsKnownPlugin(prefix)) {
    return device;
  }
  return plugin + device.substr(colon);
}

}  // namespace AlsaPlugin

#endif
