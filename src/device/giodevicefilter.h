#ifndef STRAWBERRY_GIODEVICEFILTER_H
#define STRAWBERRY_GIODEVICEFILTER_H

#include <cstring>

namespace GioDeviceFilter {

inline bool IsSuitableFilesystem(const char *filesystem_type) {
  if (!filesystem_type || !*filesystem_type) {
    return true;
  }
  return std::strcmp(filesystem_type, "udf") != 0 && std::strcmp(filesystem_type, "smb") != 0 &&
         std::strcmp(filesystem_type, "cifs") != 0 && std::strcmp(filesystem_type, "ssh") != 0 &&
         std::strcmp(filesystem_type, "isofs") != 0;
}

inline bool IsSuitable(bool has_volume, bool system_internal, bool has_drive, bool drive_removable, const char *filesystem_type) {
  if (!has_volume || system_internal) {
    return false;
  }
  if (has_drive && !drive_removable) {
    return false;
  }
  return IsSuitableFilesystem(filesystem_type);
}

}  // namespace GioDeviceFilter

#endif  // STRAWBERRY_GIODEVICEFILTER_H
