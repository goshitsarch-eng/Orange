#include "osd/osdmac.h"

#ifdef __APPLE__
#include "core/logging.h"

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

bool OSDMac::SupportsNativeNotifications() {
  return OSDMacNative::NotificationCenterAvailable([UNUserNotificationCenter currentNotificationCenter] != nil);
}

bool OSDMac::SupportsTrayPopups() { return OSDMacNative::SupportsTrayPopups(); }

void OSDMac::ShowMessageNative(const std::string &summary, const std::string &body, const std::string &icon,
                               const std::vector<unsigned char> &art) {
  (void)icon;
  (void)art;
  if (!SupportsNativeNotifications()) {
    return;
  }

  UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
  [content setTitle:[NSString stringWithUTF8String:summary.c_str()]];
  [content setSubtitle:[NSString stringWithUTF8String:body.c_str()]];

  UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:[[NSUUID UUID] UUIDString] content:content trigger:nil];
  [[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:request withCompletionHandler:^(NSError *error) {
    if (error) {
      const char *desc = error.localizedDescription.UTF8String;
      LogWarning("Failed to deliver notification: %s", desc ? desc : "unknown");
    }
  }];
  [content release];
}

#endif
