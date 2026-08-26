#ifndef STRAWBERRY_DEVICEPROPERTIESICONS_H
#define STRAWBERRY_DEVICEPROPERTIESICONS_H

#include <cstring>
#include <string>
#include <vector>

namespace DevicePropertiesIcons {

inline const std::vector<const char *> &Names() {
  static const std::vector<const char *> kNames = {"device",          "device-usb-drive", "device-usb-flash", "media-optical",
                                                   "device-ipod",     "device-ipod-nano", "device-phone"};
  return kNames;
}

inline const char *GtkName(const char *qt_name) {
  if (!qt_name || qt_name[0] == '\0' || std::strcmp(qt_name, "device") == 0) {
    return "drive-harddisk-symbolic";
  }
  if (std::strcmp(qt_name, "device-usb-drive") == 0) {
    return "drive-harddisk-usb-symbolic";
  }
  if (std::strcmp(qt_name, "device-usb-flash") == 0) {
    return "media-flash-symbolic";
  }
  if (std::strcmp(qt_name, "media-optical") == 0) {
    return "media-optical-symbolic";
  }
  if (std::strcmp(qt_name, "device-ipod") == 0 || std::strcmp(qt_name, "device-ipod-nano") == 0) {
    return "multimedia-player-symbolic";
  }
  if (std::strcmp(qt_name, "device-phone") == 0) {
    return "phone-symbolic";
  }
  return qt_name;
}

inline int IndexOf(const std::string &icon_name) {
  const auto &names = Names();
  for (size_t i = 0; i < names.size(); ++i) {
    if (icon_name == names[i]) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

inline const char *IconAt(int index) {
  const auto &names = Names();
  if (index < 0 || index >= static_cast<int>(names.size())) {
    return names.front();
  }
  return names[static_cast<size_t>(index)];
}

inline std::string EffectiveIcon(const std::string &icon_name) {
  return IndexOf(icon_name) >= 0 ? icon_name : Names().front();
}

}  // namespace DevicePropertiesIcons

#endif
