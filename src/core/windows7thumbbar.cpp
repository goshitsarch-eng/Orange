#include "core/windows7thumbbar.h"

#ifdef _WIN32
#include "core/logging.h"
#include "utilities/winutils.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include <commctrl.h>
#include <shobjidl.h>
#include <windows.h>

#include <string>

namespace {

constexpr const wchar_t *kThumbBarProp = L"StrawberryThumbBar";
constexpr const wchar_t *kThumbBarOldProcProp = L"StrawberryThumbBarOld";

HICON IconFromName(const char *name) {
  if (!name || !name[0]) {
    return nullptr;
  }
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    return nullptr;
  }
  GtkIconTheme *theme = gtk_icon_theme_get_for_display(display);
  GtkIconPaintable *paintable = gtk_icon_theme_lookup_icon(theme, name, nullptr, Windows7ThumbBarActions::kIconSize, 1, GTK_TEXT_DIR_NONE,
                                                           static_cast<GtkIconLookupFlags>(0));
  if (!paintable) {
    return nullptr;
  }
  GFile *file = gtk_icon_paintable_get_file(paintable);
  g_object_unref(paintable);
  if (!file) {
    return nullptr;
  }
  char *path = g_file_get_path(file);
  g_object_unref(file);
  if (!path) {
    return nullptr;
  }
  GError *error = nullptr;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(path, Windows7ThumbBarActions::kIconSize, Windows7ThumbBarActions::kIconSize, &error);
  g_free(path);
  if (error) {
    g_error_free(error);
  }
  if (!pixbuf) {
    return nullptr;
  }

  const int width = gdk_pixbuf_get_width(pixbuf);
  const int height = gdk_pixbuf_get_height(pixbuf);
  BITMAPV5HEADER header{};
  header.bV5Size = sizeof(BITMAPV5HEADER);
  header.bV5Width = width;
  header.bV5Height = -height;
  header.bV5Planes = 1;
  header.bV5BitCount = 32;
  header.bV5Compression = BI_BITFIELDS;
  header.bV5RedMask = 0x00FF0000;
  header.bV5GreenMask = 0x0000FF00;
  header.bV5BlueMask = 0x000000FF;
  header.bV5AlphaMask = 0xFF000000;
  void *bits = nullptr;
  HDC hdc = GetDC(nullptr);
  HBITMAP color = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
  ReleaseDC(nullptr, hdc);
  if (!color || !bits) {
    g_object_unref(pixbuf);
    return nullptr;
  }
  const guchar *src = gdk_pixbuf_get_pixels(pixbuf);
  const int stride = gdk_pixbuf_get_rowstride(pixbuf);
  const int channels = gdk_pixbuf_get_n_channels(pixbuf);
  auto *dest = static_cast<unsigned char *>(bits);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const guchar *pixel = src + y * stride + x * channels;
      dest[(y * width + x) * 4 + 0] = pixel[2];
      dest[(y * width + x) * 4 + 1] = pixel[1];
      dest[(y * width + x) * 4 + 2] = pixel[0];
      dest[(y * width + x) * 4 + 3] = channels >= 4 ? pixel[3] : 255;
    }
  }
  g_object_unref(pixbuf);
  HBITMAP mask = CreateBitmap(width, height, 1, 1, nullptr);
  ICONINFO info{};
  info.fIcon = TRUE;
  info.hbmMask = mask;
  info.hbmColor = color;
  HICON icon = CreateIconIndirect(&info);
  DeleteObject(color);
  DeleteObject(mask);
  return icon;
}

void CopyTooltip(const char *text, wchar_t *out, int out_chars) {
  if (!out || out_chars <= 0) {
    return;
  }
  out[0] = L'\0';
  if (!text || !text[0]) {
    return;
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
  if (n <= 0) {
    return;
  }
  std::wstring wide(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), n);
  const int copy = Windows7ThumbBarActions::ClampTipChars(static_cast<int>(wide.size() ? wide.size() - 1 : 0));
  const int limit = copy < out_chars ? copy : out_chars - 1;
  for (int i = 0; i < limit; ++i) {
    out[i] = wide[static_cast<size_t>(i)];
  }
  out[limit] = L'\0';
}

LRESULT CALLBACK ThumbBarWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto *self = static_cast<Windows7ThumbBar *>(GetPropW(hwnd, kThumbBarProp));
  WNDPROC old_proc = reinterpret_cast<WNDPROC>(GetPropW(hwnd, kThumbBarOldProcProp));
  if (!old_proc) {
    old_proc = DefWindowProcW;
  }
  if (self) {
    MSG message{};
    message.hwnd = hwnd;
    message.message = msg;
    message.wParam = wparam;
    message.lParam = lparam;
    self->HandleWinEvent(&message);
  }
  return CallWindowProcW(old_proc, hwnd, msg, wparam, lparam);
}

}  // namespace

Windows7ThumbBar::Windows7ThumbBar(GtkWidget *window) : window_(window) {}

Windows7ThumbBar::~Windows7ThumbBar() {
  if (update_source_) {
    g_source_remove(update_source_);
    update_source_ = 0;
  }
  HWND hwnd = static_cast<HWND>(WinUtils::NativeHandle(window_));
  if (hwnd && old_wndproc_) {
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(old_wndproc_));
    RemovePropW(hwnd, kThumbBarProp);
    RemovePropW(hwnd, kThumbBarOldProcProp);
    old_wndproc_ = nullptr;
  }
  if (taskbar_list_) {
    static_cast<ITaskbarList3 *>(taskbar_list_)->Release();
    taskbar_list_ = nullptr;
  }
}

void Windows7ThumbBar::SetActions(const std::vector<Windows7ThumbBarActions::Id> &actions) {
  actions_ = actions;
  if (actions_.size() > static_cast<size_t>(Windows7ThumbBarActions::kMaxButtonCount)) {
    actions_.resize(static_cast<size_t>(Windows7ThumbBarActions::kMaxButtonCount));
  }
  HWND hwnd = static_cast<HWND>(WinUtils::NativeHandle(window_));
  if (hwnd && !buttons_added_) {
    Rebuild(true);
  } else if (buttons_added_) {
    ScheduleUpdate();
  }
}

void Windows7ThumbBar::SetPlaying(bool playing) {
  if (!Windows7ThumbBarActions::ShouldRebuildOnPlayingChange(playing_, playing)) {
    return;
  }
  playing_ = playing;
  if (buttons_added_) {
    ScheduleUpdate();
  }
}

void Windows7ThumbBar::HandleCommand(int button_id) {
  const Windows7ThumbBarActions::Id id = Windows7ThumbBarActions::ActionAtCommand(button_id, actions_);
  if (!Windows7ThumbBarActions::ShouldDispatch(id) || !activated_) {
    return;
  }
  activated_(id);
}

void Windows7ThumbBar::HandleWinEvent(void *msg_void) {
  auto *msg = static_cast<MSG *>(msg_void);
  if (!msg) {
    return;
  }
  if (button_created_message_id_ == 0) {
    button_created_message_id_ = RegisterWindowMessageW(L"TaskbarButtonCreated");
  }
  const auto kind = Windows7ThumbBarActions::ClassifyMessage(msg->message, button_created_message_id_);
  if (kind == Windows7ThumbBarActions::WinMessage::TaskbarCreated) {
    if (taskbar_list_) {
      static_cast<ITaskbarList3 *>(taskbar_list_)->Release();
      taskbar_list_ = nullptr;
    }
    buttons_added_ = false;
    Rebuild(true);
    return;
  }
  if (kind == Windows7ThumbBarActions::WinMessage::Command) {
    HandleCommand(Windows7ThumbBarActions::CommandId(static_cast<unsigned>(msg->wParam)));
  }
}

void Windows7ThumbBar::ScheduleUpdate() {
  if (update_source_ != 0) {
    return;
  }
  update_source_ = g_timeout_add(Windows7ThumbBarActions::kUpdateDelayMs, +[](gpointer data) -> gboolean {
    auto *self = static_cast<Windows7ThumbBar *>(data);
    self->update_source_ = 0;
    self->Rebuild(false);
    return G_SOURCE_REMOVE;
  }, this);
}

void Windows7ThumbBar::InstallHook() {
  HWND hwnd = static_cast<HWND>(WinUtils::NativeHandle(window_));
  if (!hwnd || old_wndproc_) {
    return;
  }
  SetPropW(hwnd, kThumbBarProp, this);
  old_wndproc_ = reinterpret_cast<void *>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ThumbBarWndProc)));
  SetPropW(hwnd, kThumbBarOldProcProp, old_wndproc_);
}

void Windows7ThumbBar::SetupButton(int index, void *button_void) {
  auto *button = static_cast<THUMBBUTTON *>(button_void);
  const Windows7ThumbBarActions::Id id = actions_[static_cast<size_t>(index)];
  button->iId = static_cast<UINT>(index);
  const bool spacer = Windows7ThumbBarActions::IsSpacer(id);
  button->dwMask = static_cast<THUMBBUTTONMASK>(Windows7ThumbBarActions::ButtonMask(spacer));
  button->dwFlags = static_cast<THUMBBUTTONFLAGS>(
      Windows7ThumbBarActions::WinFlags(Windows7ThumbBarActions::FlagFor(spacer, true, true)));
  button->hIcon = nullptr;
  button->szTip[0] = L'\0';
  if (spacer) {
    return;
  }
  button->hIcon = IconFromName(Windows7ThumbBarActions::IconName(id, playing_));
  CopyTooltip(Windows7ThumbBarActions::Tooltip(id), button->szTip, 260);
}

void Windows7ThumbBar::Rebuild(bool add_buttons) {
  HWND hwnd = static_cast<HWND>(WinUtils::NativeHandle(window_));
  if (!hwnd || actions_.empty()) {
    return;
  }
  InstallHook();
  if (!taskbar_list_) {
    ITaskbarList3 *list = nullptr;
    static const GUID clsid = {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}};
    if (CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_ITaskbarList3, reinterpret_cast<void **>(&list)) != S_OK || !list) {
      LogWarning("Windows7ThumbBar: ITaskbarList3 unavailable");
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
    SetupButton(static_cast<int>(i), &buttons[i]);
  }
  const bool add = add_buttons || Windows7ThumbBarActions::ShouldAddButtons(true, buttons_added_);
  HRESULT hr = S_OK;
  if (add) {
    hr = list->ThumbBarAddButtons(hwnd, count, buttons);
    if (hr == S_OK) {
      buttons_added_ = true;
    }
  } else if (Windows7ThumbBarActions::ShouldUpdateButtons(buttons_added_)) {
    hr = list->ThumbBarUpdateButtons(hwnd, count, buttons);
  }
  if (hr != S_OK) {
    LogDebug("Windows7ThumbBar: button update failed 0x%08lx", static_cast<unsigned long>(hr));
  }
  for (UINT i = 0; i < count; ++i) {
    if (buttons[i].hIcon) {
      DestroyIcon(buttons[i].hIcon);
    }
  }
}
#endif
