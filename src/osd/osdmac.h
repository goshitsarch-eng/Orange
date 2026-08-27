#ifndef STRAWBERRY_OSDMAC_H
#define STRAWBERRY_OSDMAC_H

#include <string>
#include <vector>

namespace OSDMacNative {

// Qt OSDMac::SupportsNativeNotifications is true when UNUserNotificationCenter exists.
inline bool NotificationCenterAvailable(bool center_present) { return center_present; }

// Qt OSDMac::SupportsTrayPopups is always false.
inline bool SupportsTrayPopups() { return false; }

}  // namespace OSDMacNative

#ifdef __APPLE__
class OSDMac {
 public:
  static bool SupportsNativeNotifications();
  static bool SupportsTrayPopups();
  static void ShowMessageNative(const std::string &summary, const std::string &body, const std::string &icon = {},
                                const std::vector<unsigned char> &art = {});
};
#endif

#endif
