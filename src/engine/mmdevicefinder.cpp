#include "engine/mmdevicefinder.h"

#include "config.h"
#include "core/logging.h"
#include "engine/platformdeviceoutputs.h"

#ifdef _WIN32
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>
#ifdef _MSC_VER
#include <functiondiscoverykeys.h>
#else
#include <functiondiscoverykeys_devpkey.h>
#endif
#include <mmdeviceapi.h>

#ifdef _MSC_VER
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
#endif
#endif

MMDeviceFinder::MMDeviceFinder() : DeviceFinder("mmdevice", {"wasapisink", "wasapi2sink"}) {}

EngineDeviceList MMDeviceFinder::ListDevices() {
  EngineDeviceList devices;
  EngineDevice default_device;
  default_device.description = PlatformDeviceOutputs::DefaultDeviceDescription();
  default_device.iconname = default_device.GuessIconName();
  devices.push_back(default_device);
#ifdef _WIN32
  const HRESULT hr_coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  IMMDeviceEnumerator *enumerator = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void **>(&enumerator));
  if (hr == S_OK) {
    IMMDeviceCollection *collection = nullptr;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (hr == S_OK) {
      UINT count = 0;
      hr = collection->GetCount(&count);
      if (hr == S_OK) {
        for (ULONG i = 0; i < count; ++i) {
          IMMDevice *endpoint = nullptr;
          hr = collection->Item(i, &endpoint);
          if (hr != S_OK) {
            LogError("IMMDeviceCollection::Item failed.");
            continue;
          }
          LPWSTR pwszid = nullptr;
          hr = endpoint->GetId(&pwszid);
          if (hr == S_OK) {
            IPropertyStore *props = nullptr;
            hr = endpoint->OpenPropertyStore(STGM_READ, &props);
            if (hr == S_OK) {
              PROPVARIANT var_name;
              PropVariantInit(&var_name);
              hr = props->GetValue(PKEY_Device_FriendlyName, &var_name);
              if (hr == S_OK && var_name.pwszVal) {
                EngineDevice device;
                const int needed = WideCharToMultiByte(CP_UTF8, 0, var_name.pwszVal, -1, nullptr, 0, nullptr, nullptr);
                std::string name(needed > 0 ? static_cast<size_t>(needed - 1) : 0, '\0');
                if (needed > 1) {
                  WideCharToMultiByte(CP_UTF8, 0, var_name.pwszVal, -1, name.data(), needed, nullptr, nullptr);
                }
                device.description = name;
                const int id_needed = WideCharToMultiByte(CP_UTF8, 0, pwszid, -1, nullptr, 0, nullptr, nullptr);
                std::string id(id_needed > 0 ? static_cast<size_t>(id_needed - 1) : 0, '\0');
                if (id_needed > 1) {
                  WideCharToMultiByte(CP_UTF8, 0, pwszid, -1, id.data(), id_needed, nullptr, nullptr);
                }
                device.value = id;
                device.iconname = device.GuessIconName();
                devices.push_back(device);
              }
              PropVariantClear(&var_name);
              props->Release();
            }
            CoTaskMemFree(pwszid);
          }
          endpoint->Release();
        }
      }
      collection->Release();
    }
    enumerator->Release();
  }
  if (hr_coinit == S_OK || hr_coinit == S_FALSE) {
    CoUninitialize();
  }
#endif
  return devices;
}
