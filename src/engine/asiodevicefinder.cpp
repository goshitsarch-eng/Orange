#include "engine/asiodevicefinder.h"

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

AsioDeviceFinder::AsioDeviceFinder() : DeviceFinder("asio", {"asiosink"}) {}

EngineDeviceList AsioDeviceFinder::ListDevices() {
  EngineDeviceList devices;
#ifdef _WIN32
  HKEY reg_key = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"software\\asio", 0, KEY_READ, &reg_key) != ERROR_SUCCESS) {
    return devices;
  }
  for (DWORD i = 0;; ++i) {
    WCHAR key_name[256]{};
    const LSTATUS status = RegEnumKeyW(reg_key, i, key_name, sizeof(key_name) / sizeof(key_name[0]));
    if (status != ERROR_SUCCESS) {
      break;
    }
    HKEY sub_key = nullptr;
    if (RegOpenKeyExW(reg_key, key_name, 0, KEY_READ, &sub_key) != ERROR_SUCCESS) {
      continue;
    }
    WCHAR clsid_data[256]{};
    DWORD clsid_size = sizeof(clsid_data);
    DWORD type = REG_SZ;
    if (RegQueryValueExW(sub_key, L"clsid", nullptr, &type, reinterpret_cast<LPBYTE>(clsid_data), &clsid_size) == ERROR_SUCCESS) {
      EngineDevice device;
      const int needed = WideCharToMultiByte(CP_UTF8, 0, key_name, -1, nullptr, 0, nullptr, nullptr);
      std::string name(needed > 0 ? static_cast<size_t>(needed - 1) : 0, '\0');
      if (needed > 1) {
        WideCharToMultiByte(CP_UTF8, 0, key_name, -1, name.data(), needed, nullptr, nullptr);
      }
      device.description = name;
      const int id_needed = WideCharToMultiByte(CP_UTF8, 0, clsid_data, -1, nullptr, 0, nullptr, nullptr);
      std::string id(id_needed > 0 ? static_cast<size_t>(id_needed - 1) : 0, '\0');
      if (id_needed > 1) {
        WideCharToMultiByte(CP_UTF8, 0, clsid_data, -1, id.data(), id_needed, nullptr, nullptr);
      }
      device.value = id;
      device.iconname = device.GuessIconName();
      if (!device.value.empty()) {
        devices.push_back(device);
      }
    }
    RegCloseKey(sub_key);
  }
  RegCloseKey(reg_key);
#endif
  return devices;
}
