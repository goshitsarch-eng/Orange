#include "core/windows7thumbbar.h"

#ifdef _WIN32
#include "utilities/winutils.h"

#include <commctrl.h>
#include <shobjidl.h>
#include <windows.h>

Windows7ThumbBar::Windows7ThumbBar(GtkWidget *window) : window_(window) {}

Windows7ThumbBar::~Windows7ThumbBar() {
  if (taskbar_list_) {
    static_cast<ITaskbarList3 *>(taskbar_list_)->Release();
  }
}

void Windows7ThumbBar::SetActions(const std::vector<Windows7ThumbBarActions::Id> &actions) {
  actions_ = actions;
  if (actions_.size() > static_cast<size_t>(Windows7ThumbBarActions::kMaxButtonCount)) {
    actions_.resize(static_cast<size_t>(Windows7ThumbBarActions::kMaxButtonCount));
  }
  Rebuild();
}

void Windows7ThumbBar::SetPlaying(bool playing) {
  if (playing_ == playing) {
    return;
  }
  playing_ = playing;
  Rebuild();
}

void Windows7ThumbBar::HandleWinEvent(void *msg_void) {
  auto *msg = static_cast<MSG *>(msg_void);
  if (!msg) {
    return;
  }
  if (button_created_message_id_ == 0) {
    button_created_message_id_ = RegisterWindowMessageW(L"TaskbarButtonCreated");
  }
  if (msg->message == button_created_message_id_) {
    Rebuild();
  }
}

void Windows7ThumbBar::Rebuild() {
  HWND hwnd = static_cast<HWND>(WinUtils::NativeHandle(window_));
  if (!hwnd) {
    return;
  }
  if (!taskbar_list_) {
    ITaskbarList3 *list = nullptr;
    static const GUID clsid = {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
    if (CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_ITaskbarList3, reinterpret_cast<void **>(&list)) != S_OK || !list) {
      return;
    }
    if (list->HrInit() != S_OK) {
      list->Release();
      return;
    }
    taskbar_list_ = list;
  }
  auto *list = static_cast<ITaskbarList3 *>(taskbar_list_);
  THUMBBUTTON buttons[Windows7ThumbBarActions::kMaxButtonCount]{};
  const UINT count = static_cast<UINT>(actions_.size());
  for (UINT i = 0; i < count; ++i) {
    const Windows7ThumbBarActions::Id id = actions_[i];
    buttons[i].iId = i;
    buttons[i].dwMask = static_cast<THUMBBUTTONMASK>(THB_FLAGS | THB_TOOLTIP);
    if (Windows7ThumbBarActions::IsSpacer(id)) {
      buttons[i].dwFlags = THBF_NOBACKGROUND;
      continue;
    }
    buttons[i].dwFlags = THBF_ENABLED;
  }
  list->ThumbBarAddButtons(hwnd, count, buttons);
}
#endif
