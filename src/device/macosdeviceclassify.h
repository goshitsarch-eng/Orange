#ifndef STRAWBERRY_MACOSDEVICECLASSIFY_H
#define STRAWBERRY_MACOSDEVICECLASSIFY_H

#include <cstdint>
#include <cstring>
#include <string>

namespace MacOsDeviceClassify {

inline constexpr const char *kIOCDMediaClass = "IOCDMedia";
inline constexpr const char *kMTPPrefix = "MTP/";
inline constexpr uint16_t kAppleVendorId = 0x05ac;
inline constexpr uint16_t kIlok2VendorId = 0x088e;
inline constexpr uint16_t kIlok2ProductId = 0x5036;
inline constexpr uint16_t kElicenserVendorId = 0x0819;
inline constexpr uint16_t kElicenserProductId = 0x0101;
inline constexpr int kUsbHidClass = 3;
inline constexpr int kUsbPrinterClass = 7;
inline constexpr int kUsbHubClass = 9;

inline bool IsCDKind(const char *kind) { return kind && std::strcmp(kind, kIOCDMediaClass) == 0; }

inline bool IsCDKind(const std::string &kind) { return IsCDKind(kind.c_str()); }

inline bool IsMTPUniqueId(const std::string &id) { return id.rfind(kMTPPrefix, 0) == 0 || id.rfind("MTP", 0) == 0; }

inline std::string MTPUniqueId(const std::string &serial) {
  if (serial.rfind(kMTPPrefix, 0) == 0) {
    return serial;
  }
  return std::string(kMTPPrefix) + serial;
}

inline std::string MTPSerial(const std::string &unique_id) {
  if (unique_id.rfind(kMTPPrefix, 0) == 0) {
    return unique_id.substr(4);
  }
  if (unique_id.rfind("mtp:", 0) == 0) {
    return unique_id.substr(4);
  }
  return unique_id;
}

inline std::string FriendlyName(const std::string &vendor, const std::string &product) {
  if (vendor.empty()) {
    return product.empty() ? "MTP device" : product;
  }
  if (product.empty()) {
    return vendor;
  }
  return vendor + " " + product;
}

inline bool ShouldSkipUsbDevice(uint16_t vendor_id, uint16_t product_id, int interface_class) {
  if (vendor_id == kAppleVendorId) {
    return true;
  }
  if (vendor_id == kIlok2VendorId && product_id == kIlok2ProductId) {
    return true;
  }
  if (vendor_id == kElicenserVendorId && product_id == kElicenserProductId) {
    return true;
  }
  return interface_class == kUsbHidClass || interface_class == kUsbPrinterClass || interface_class == kUsbHubClass;
}

inline bool MatchesMtpDevice(uint16_t vendor_id, uint16_t product_id, uint16_t want_vendor, uint16_t want_product) {
  return vendor_id == want_vendor && product_id == want_product;
}

inline std::string RawDevicePath(const std::string &bsd) {
  if (bsd.rfind("rdisk", 0) == 0) {
    return "/dev/" + bsd;
  }
  if (bsd.rfind("disk", 0) == 0) {
    return "/dev/r" + bsd;
  }
  return bsd.empty() ? std::string() : "/dev/" + bsd;
}

inline const char *IconForKind(bool is_cd, bool is_mtp) {
  if (is_cd) {
    return "media-optical-symbolic";
  }
  if (is_mtp) {
    return "multimedia-player-symbolic";
  }
  return "drive-removable-media-symbolic";
}

inline const char *BackendForKind(bool is_cd, bool is_mtp) {
  if (is_cd) {
    return "cdda";
  }
  if (is_mtp) {
    return "mtp";
  }
  return "macos";
}

}  // namespace MacOsDeviceClassify

#endif
