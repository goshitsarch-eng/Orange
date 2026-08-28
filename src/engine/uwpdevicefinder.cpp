#include "config.h"

#include "engine/uwpdevicefinder.h"

#include "core/logging.h"
#include "engine/platformdeviceoutputs.h"
#include "engine/uwpdeviceenum.h"

#include <string>

#ifdef _WIN32
#include <windows.h>
#if defined(_MSC_VER)
#include <roapi.h>
#include <winstring.h>
#include <wrl.h>
#include <windows.devices.enumeration.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#endif
#endif

UWPDeviceFinder::UWPDeviceFinder() : DeviceFinder(UwpDeviceEnum::kFinderName, {UwpDeviceEnum::kOutput}) {}

#if defined(_WIN32) && defined(_MSC_VER)
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Devices::Enumeration;

namespace {

std::string WideToUtf8(const wchar_t *wide) {
  if (!wide || !wide[0]) {
    return {};
  }
  const int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
  if (n <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(n - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), n, nullptr, nullptr);
  return out;
}

std::string HStringToUtf8(HString *hstr) {
  if (!hstr) {
    return {};
  }
  const wchar_t *raw = hstr->GetRawBuffer(nullptr);
  return WideToUtf8(raw);
}

HRESULT SyncWaitCollection(IAsyncOperation<DeviceInformationCollection *> *op) {
  if (!op) {
    return E_POINTER;
  }
  HANDLE ev = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!ev) {
    return E_FAIL;
  }
  HRESULT wait_hr = E_FAIL;
  auto handler = Callback<IAsyncOperationCompletedHandler<DeviceInformationCollection *>>(
      [ev, &wait_hr](IAsyncOperation<DeviceInformationCollection *> *, AsyncStatus status) -> HRESULT {
        wait_hr = (status == AsyncStatus::Completed) ? S_OK : E_FAIL;
        SetEvent(ev);
        return S_OK;
      });
  const HRESULT hr = op->put_Completed(handler.Get());
  if (FAILED(hr)) {
    CloseHandle(ev);
    return hr;
  }
  WaitForSingleObject(ev, 5000);
  CloseHandle(ev);
  return wait_hr;
}

}  // namespace
#endif

EngineDeviceList UWPDeviceFinder::ListDevices() {
  EngineDeviceList devices;
  devices.push_back(UwpDeviceEnum::DefaultDevice());
#if defined(_WIN32) && defined(_MSC_VER)
  ComPtr<IDeviceInformationStatics> device_info_statics;
  HRESULT hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Devices_Enumeration_DeviceInformation).Get(), &device_info_statics);
  if (FAILED(hr) || !device_info_statics) {
    LogWarning("UWPDeviceFinder: DeviceInformation factory unavailable 0x%08lx", static_cast<unsigned long>(hr));
    return devices;
  }

  ComPtr<IAsyncOperation<DeviceInformationCollection *>> async_op;
  hr = device_info_statics->FindAllAsyncDeviceClass(static_cast<DeviceClass>(UwpDeviceEnum::kAudioRenderClass), &async_op);
  if (FAILED(hr) || !async_op) {
    LogWarning("UWPDeviceFinder: FindAllAsyncDeviceClass failed 0x%08lx", static_cast<unsigned long>(hr));
    return devices;
  }

  hr = SyncWaitCollection(async_op.Get());
  if (FAILED(hr)) {
    LogWarning("UWPDeviceFinder: device enumeration timed out or failed");
    return devices;
  }

  ComPtr<IVectorView<DeviceInformation *>> device_list;
  hr = async_op->GetResults(&device_list);
  if (FAILED(hr) || !device_list) {
    return devices;
  }

  unsigned int count = 0;
  hr = device_list->get_Size(&count);
  if (FAILED(hr)) {
    return devices;
  }

  for (unsigned int i = 0; i < count; ++i) {
    ComPtr<IDeviceInformation> device_info;
    if (FAILED(device_list->GetAt(i, &device_info)) || !device_info) {
      continue;
    }
    boolean enabled = false;
    if (FAILED(device_info->get_IsEnabled(&enabled)) || !UwpDeviceEnum::ShouldInclude(enabled != 0)) {
      continue;
    }
    HString id;
    if (FAILED(device_info->get_Id(id.GetAddressOf())) || !id.IsValid()) {
      continue;
    }
    HString name;
    if (FAILED(device_info->get_Name(name.GetAddressOf())) || !name.IsValid()) {
      continue;
    }
    EngineDevice device = UwpDeviceEnum::FromWinRt(HStringToUtf8(&id), HStringToUtf8(&name));
    device.iconname = device.GuessIconName();
    devices.push_back(device);
  }
#elif defined(_WIN32)
  (void)PlatformDeviceOutputs::DefaultDeviceDescription();
#endif
  return devices;
}
