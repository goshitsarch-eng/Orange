#include "engine/enginedevice.h"

#include "utilities/strutils.h"

std::string EngineDevice::GuessIconName() const {
  const std::string text = StrUtils::ToLower(description + " " + value);
  if (StrUtils::ContainsInsensitive(text, "hdmi") || StrUtils::ContainsInsensitive(text, "display")) {
    return "video-display-symbolic";
  }
  if (StrUtils::ContainsInsensitive(text, "usb") || StrUtils::ContainsInsensitive(text, "headset") || StrUtils::ContainsInsensitive(text, "headphone")) {
    return "audio-headphones-symbolic";
  }
  if (StrUtils::ContainsInsensitive(text, "bluetooth")) {
    return "bluetooth-symbolic";
  }
  return "audio-card-symbolic";
}
