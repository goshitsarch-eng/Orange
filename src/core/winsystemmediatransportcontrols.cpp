#include "config.h"

#include "core/winsystemmediatransportcontrols.h"

#include "core/logging.h"
#include "core/player.h"

#include <glib.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#if defined(_MSC_VER)
#include <eventtoken.h>
#include <inspectable.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.foundation.h>
#include <windows.media.h>
#include <windows.storage.streams.h>
#if __has_include(<wrl.h>)
#include <wrl.h>
#else
#include <wrl/client.h>
#include <wrl/event.h>
#endif
#include <systemmediatransportcontrolsinterop.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#endif

#if defined(_WIN32) && defined(_MSC_VER)
using namespace ABI::Windows::Media;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;

namespace {

HRESULT CreateHString(const std::string &str, HSTRING *out) {
  if (str.empty()) {
    return WindowsCreateString(L"", 0, out);
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
  if (n <= 0) {
    return E_FAIL;
  }
  std::wstring wide(static_cast<size_t>(n - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide.data(), n);
  return WindowsCreateString(wide.c_str(), static_cast<UINT32>(wide.size()), out);
}

std::vector<unsigned char> EncodeJpeg(const std::vector<unsigned char> &image) {
  if (WinSmtcStatus::LooksLikeJpeg(image)) {
    return image;
  }
  GError *error = nullptr;
  GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
  if (!gdk_pixbuf_loader_write(loader, image.data(), image.size(), &error)) {
    if (error) {
      g_error_free(error);
    }
    g_object_unref(loader);
    return {};
  }
  gdk_pixbuf_loader_close(loader, nullptr);
  GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
  if (!pixbuf) {
    g_object_unref(loader);
    return {};
  }
  gchar *out = nullptr;
  gsize out_len = 0;
  if (!gdk_pixbuf_save_to_buffer(pixbuf, &out, &out_len, "jpeg", nullptr, "quality", "90", nullptr)) {
    g_object_unref(loader);
    return {};
  }
  std::vector<unsigned char> jpeg(out, out + out_len);
  g_free(out);
  g_object_unref(loader);
  return jpeg;
}

struct ButtonIdle {
  WinSystemMediaTransportControls *self = nullptr;
  int button = 0;
  std::shared_ptr<bool> alive;
};

gboolean EmitButtonIdle(gpointer data) {
  auto *idle = static_cast<ButtonIdle *>(data);
  if (idle->alive && *idle->alive) {
    idle->self->HandleButtonPressed(idle->button);
  }
  delete idle;
  return G_SOURCE_REMOVE;
}

using SmtcBtnHandlerType = ITypedEventHandler<SystemMediaTransportControls *, SystemMediaTransportControlsButtonPressedEventArgs *>;

}  // namespace
#endif

WinSystemMediaTransportControls::WinSystemMediaTransportControls(Player *player) : player_(player) {}

WinSystemMediaTransportControls::~WinSystemMediaTransportControls() {
  *alive_ = false;
  StopTimelineTimer();
#if defined(_WIN32) && defined(_MSC_VER)
  ISystemMediaTransportControls *smtc = static_cast<ISystemMediaTransportControls *>(smtc_);
  if (smtc && button_pressed_token_) {
    EventRegistrationToken token;
    token.value = button_pressed_token_;
    smtc->remove_ButtonPressed(token);
  }
  if (button_handler_) {
    static_cast<IUnknown *>(button_handler_)->Release();
    button_handler_ = nullptr;
  }
  if (updater_) {
    static_cast<ISystemMediaTransportControlsDisplayUpdater *>(updater_)->Release();
    updater_ = nullptr;
  }
  if (smtc2_) {
    static_cast<ISystemMediaTransportControls2 *>(smtc2_)->Release();
    smtc2_ = nullptr;
  }
  if (smtc) {
    smtc->put_IsEnabled(false);
    smtc->Release();
    smtc_ = nullptr;
  }
  if (ro_initialized_) {
    RoUninitialize();
    ro_initialized_ = false;
  }
#endif
}

bool WinSystemMediaTransportControls::Initialize(void *hwnd) {
#if defined(_WIN32) && defined(_MSC_VER)
  if (!hwnd) {
    return false;
  }
  const HRESULT hr_init = RoInitialize(RO_INIT_SINGLETHREADED);
  if (hr_init != S_OK && hr_init != S_FALSE) {
    LogError("WinSystemMediaTransportControls: RoInitialize failed 0x%08lx", static_cast<unsigned long>(hr_init));
    return false;
  }
  ro_initialized_ = true;

  HSTRING h_class = nullptr;
  static const wchar_t kSmtcClass[] = L"Windows.Media.SystemMediaTransportControls";
  WindowsCreateString(kSmtcClass, static_cast<UINT32>(wcslen(kSmtcClass)), &h_class);

  ISystemMediaTransportControlsInterop *interop = nullptr;
  HRESULT hr = RoGetActivationFactory(h_class, __uuidof(ISystemMediaTransportControlsInterop), reinterpret_cast<void **>(&interop));
  WindowsDeleteString(h_class);
  if (FAILED(hr) || !interop) {
    LogError("WinSystemMediaTransportControls: Failed to get SMTC interop factory 0x%08lx", static_cast<unsigned long>(hr));
    return false;
  }

  ISystemMediaTransportControls *smtc = nullptr;
  hr = interop->GetForWindow(static_cast<HWND>(hwnd), __uuidof(ISystemMediaTransportControls), reinterpret_cast<void **>(&smtc));
  interop->Release();
  if (FAILED(hr) || !smtc) {
    LogError("WinSystemMediaTransportControls: GetForWindow failed 0x%08lx", static_cast<unsigned long>(hr));
    return false;
  }

  smtc->put_IsEnabled(true);
  smtc->put_IsPlayEnabled(true);
  smtc->put_IsPauseEnabled(true);
  smtc->put_IsStopEnabled(true);
  smtc->put_IsNextEnabled(true);
  smtc->put_IsPreviousEnabled(true);

  Microsoft::WRL::ComPtr<SmtcBtnHandlerType> handler = Microsoft::WRL::Callback<SmtcBtnHandlerType>(
      [this](ISystemMediaTransportControls *, ISystemMediaTransportControlsButtonPressedEventArgs *args) -> HRESULT {
        SystemMediaTransportControlsButton btn;
        if (SUCCEEDED(args->get_Button(&btn))) {
          auto *idle = new ButtonIdle;
          idle->self = this;
          idle->button = static_cast<int>(btn);
          idle->alive = alive_;
          g_idle_add(EmitButtonIdle, idle);
        }
        return S_OK;
      });

  EventRegistrationToken token{};
  hr = smtc->add_ButtonPressed(handler.Get(), &token);
  if (FAILED(hr)) {
    LogError("WinSystemMediaTransportControls: Failed to register button handler 0x%08lx", static_cast<unsigned long>(hr));
    smtc->put_IsEnabled(false);
    smtc->Release();
    return false;
  }

  ISystemMediaTransportControlsDisplayUpdater *updater = nullptr;
  hr = smtc->get_DisplayUpdater(&updater);
  if (FAILED(hr) || !updater) {
    LogError("WinSystemMediaTransportControls: Failed to get display updater 0x%08lx", static_cast<unsigned long>(hr));
    smtc->remove_ButtonPressed(token);
    smtc->put_IsEnabled(false);
    smtc->Release();
    return false;
  }

  handler->AddRef();
  button_handler_ = handler.Get();
  button_pressed_token_ = token.value;
  smtc_ = smtc;
  updater_ = updater;

  ISystemMediaTransportControls2 *smtc2 = nullptr;
  if (SUCCEEDED(smtc->QueryInterface(__uuidof(ISystemMediaTransportControls2), reinterpret_cast<void **>(&smtc2)))) {
    smtc2_ = smtc2;
  } else {
    LogWarning("WinSystemMediaTransportControls: ISystemMediaTransportControls2 unavailable, timeline disabled");
  }

  initialized_ = true;
  if (player_) {
    UpdatePlaybackStatus(player_->GetState());
    UpdateMetadata(player_->current_song());
  }
  LogInfo("WinSystemMediaTransportControls: Initialized");
  return true;
#elif defined(_WIN32)
  initialized_ = hwnd != nullptr;
  (void)hwnd;
  if (initialized_ && player_) {
    UpdatePlaybackStatus(player_->GetState());
    UpdateMetadata(player_->current_song());
  }
  return initialized_;
#else
  (void)hwnd;
  return false;
#endif
}

void WinSystemMediaTransportControls::EngineStateChanged(EngineBase::State state) {
  state_ = state;
  UpdatePlaybackStatus(state);
  if (WinSmtcStatus::ShouldRunTimelineTimer(state)) {
    StartTimelineTimer();
  } else {
    StopTimelineTimer();
    UpdateTimeline();
  }
}

void WinSystemMediaTransportControls::CurrentSongChanged(const Song &song) {
  current_song_url_ = song.url();
  current_duration_nanosec_ = song.length_nanosec();
  UpdateMetadata(song);
  UpdateTimeline();
}

void WinSystemMediaTransportControls::AlbumCoverLoaded(const Song &song, const std::vector<unsigned char> &image) {
  const bool url_matches = WinSmtcStatus::ShouldApplyCover(song.url(), current_song_url_);
  if (WinSmtcStatus::ShouldSetThumbnail(url_matches, !image.empty())) {
    SetThumbnail(image);
  } else if (WinSmtcStatus::ShouldClearThumbnail(url_matches, !image.empty())) {
    ClearThumbnail();
  }
}

void WinSystemMediaTransportControls::HandleButtonPressed(int button) {
  const WinSmtcStatus::Button id = WinSmtcStatus::ButtonFromWinRt(button);
  if (player_ && WinSmtcStatus::DispatchesToPlayer(id)) {
    switch (id) {
      case WinSmtcStatus::Button::Play:
        player_->Play();
        break;
      case WinSmtcStatus::Button::Pause:
        player_->Pause();
        break;
      case WinSmtcStatus::Button::Stop:
        player_->Stop();
        break;
      case WinSmtcStatus::Button::Next:
        player_->Next();
        break;
      case WinSmtcStatus::Button::Previous:
        player_->Previous();
        break;
      default:
        break;
    }
  }
  if (button_) {
    const char *action = WinSmtcStatus::ButtonAction(id);
    if (action && *action) {
      button_(action);
    }
  }
}

void WinSystemMediaTransportControls::StartTimelineTimer() {
  if (timeline_timer_ != 0) {
    return;
  }
  timeline_timer_ = g_timeout_add_seconds(1, +[](gpointer data) -> gboolean {
    static_cast<WinSystemMediaTransportControls *>(data)->UpdateTimeline();
    return G_SOURCE_CONTINUE;
  }, this);
}

void WinSystemMediaTransportControls::StopTimelineTimer() {
  if (timeline_timer_ == 0) {
    return;
  }
  g_source_remove(timeline_timer_);
  timeline_timer_ = 0;
}

void WinSystemMediaTransportControls::UpdateTimeline() {
#if defined(_WIN32) && defined(_MSC_VER)
  if (!smtc2_ || !player_ || !player_->engine()) {
    return;
  }
  ISystemMediaTransportControls2 *smtc2 = static_cast<ISystemMediaTransportControls2 *>(smtc2_);

  HSTRING h_class = nullptr;
  static const wchar_t kClass[] = L"Windows.Media.SystemMediaTransportControlsTimelineProperties";
  WindowsCreateString(kClass, static_cast<UINT32>(wcslen(kClass)), &h_class);
  IInspectable *insp = nullptr;
  const HRESULT hr = RoActivateInstance(h_class, &insp);
  WindowsDeleteString(h_class);
  if (FAILED(hr) || !insp) {
    return;
  }

  ISystemMediaTransportControlsTimelineProperties *props = nullptr;
  if (FAILED(insp->QueryInterface(__uuidof(ISystemMediaTransportControlsTimelineProperties), reinterpret_cast<void **>(&props))) || !props) {
    insp->Release();
    return;
  }
  insp->Release();

  const int64_t pos_ns = player_->engine()->position_nanosec();
  const int64_t dur_ns = WinSmtcStatus::TimelineDurationNs(current_duration_nanosec_, player_->engine()->length_nanosec());
  TimeSpan zero_ts = {};
  TimeSpan pos_ts = {WinSmtcStatus::TimelineHundredNs(pos_ns)};
  TimeSpan dur_ts = {WinSmtcStatus::TimelineHundredNs(dur_ns)};
  props->put_StartTime(zero_ts);
  props->put_EndTime(dur_ts);
  props->put_MinSeekTime(zero_ts);
  props->put_MaxSeekTime(dur_ts);
  props->put_Position(pos_ts);
  smtc2->UpdateTimelineProperties(props);
  props->Release();
#endif
}

void WinSystemMediaTransportControls::UpdatePlaybackStatus(EngineBase::State state) {
#if defined(_WIN32) && defined(_MSC_VER)
  if (!smtc_) {
    return;
  }
  ISystemMediaTransportControls *smtc = static_cast<ISystemMediaTransportControls *>(smtc_);
  MediaPlaybackStatus playback_status = MediaPlaybackStatus_Stopped;
  switch (WinSmtcStatus::FromEngine(state)) {
    case WinSmtcStatus::Playback::Playing:
      playback_status = MediaPlaybackStatus_Playing;
      break;
    case WinSmtcStatus::Playback::Paused:
      playback_status = MediaPlaybackStatus_Paused;
      break;
    default:
      playback_status = MediaPlaybackStatus_Stopped;
      break;
  }
  smtc->put_PlaybackStatus(playback_status);
#else
  (void)state;
#ifdef _WIN32
  LogDebug("SMTC playback status updated");
#endif
#endif
}

void WinSystemMediaTransportControls::UpdateMetadata(const Song &song) {
#if defined(_WIN32) && defined(_MSC_VER)
  if (!updater_) {
    return;
  }
  ISystemMediaTransportControlsDisplayUpdater *updater = static_cast<ISystemMediaTransportControlsDisplayUpdater *>(updater_);
  if (WinSmtcStatus::ShouldClearMetadata(song.is_valid())) {
    updater->ClearAll();
    updater->Update();
    return;
  }

  updater->put_Type(MediaPlaybackType_Music);
  IMusicDisplayProperties *music_props = nullptr;
  if (SUCCEEDED(updater->get_MusicProperties(&music_props)) && music_props) {
    HSTRING h = nullptr;
    if (SUCCEEDED(CreateHString(song.title(), &h))) {
      music_props->put_Title(h);
      WindowsDeleteString(h);
      h = nullptr;
    }
    if (SUCCEEDED(CreateHString(song.artist(), &h))) {
      music_props->put_Artist(h);
      WindowsDeleteString(h);
      h = nullptr;
    }
    if (SUCCEEDED(CreateHString(song.EffectiveAlbumartist(), &h))) {
      music_props->put_AlbumArtist(h);
      WindowsDeleteString(h);
      h = nullptr;
    }
    IMusicDisplayProperties2 *music_props2 = nullptr;
    if (SUCCEEDED(music_props->QueryInterface(__uuidof(IMusicDisplayProperties2), reinterpret_cast<void **>(&music_props2)))) {
      if (SUCCEEDED(CreateHString(song.album(), &h))) {
        music_props2->put_AlbumTitle(h);
        WindowsDeleteString(h);
      }
      music_props2->Release();
    }
    music_props->Release();
  }
  updater->Update();
#else
#ifdef _WIN32
  LogDebug("SMTC metadata %s", song.PrettyTitleWithArtist().c_str());
#else
  (void)song;
#endif
#endif
}

void WinSystemMediaTransportControls::SetThumbnail(const std::vector<unsigned char> &image) {
#if defined(_WIN32) && defined(_MSC_VER)
  if (!updater_) {
    return;
  }
  const std::vector<unsigned char> jpeg = EncodeJpeg(image);
  if (jpeg.empty()) {
    ClearThumbnail();
    return;
  }

  ISystemMediaTransportControlsDisplayUpdater *updater = static_cast<ISystemMediaTransportControlsDisplayUpdater *>(updater_);
  IRandomAccessStream *ra_stream = nullptr;
  {
    HSTRING h = nullptr;
    static const wchar_t kImsClass[] = L"Windows.Storage.Streams.InMemoryRandomAccessStream";
    WindowsCreateString(kImsClass, static_cast<UINT32>(wcslen(kImsClass)), &h);
    IInspectable *insp = nullptr;
    const HRESULT hr = RoActivateInstance(h, &insp);
    WindowsDeleteString(h);
    if (FAILED(hr) || !insp) {
      ClearThumbnail();
      return;
    }
    insp->QueryInterface(__uuidof(IRandomAccessStream), reinterpret_cast<void **>(&ra_stream));
    insp->Release();
  }
  if (!ra_stream) {
    ClearThumbnail();
    return;
  }

  IOutputStream *out = nullptr;
  ra_stream->GetOutputStreamAt(0, &out);
  if (!out) {
    ra_stream->Release();
    ClearThumbnail();
    return;
  }

  IDataWriterFactory *dwf = nullptr;
  {
    HSTRING h = nullptr;
    static const wchar_t kDwClass[] = L"Windows.Storage.Streams.DataWriter";
    WindowsCreateString(kDwClass, static_cast<UINT32>(wcslen(kDwClass)), &h);
    RoGetActivationFactory(h, __uuidof(IDataWriterFactory), reinterpret_cast<void **>(&dwf));
    WindowsDeleteString(h);
  }
  if (!dwf) {
    out->Release();
    ra_stream->Release();
    ClearThumbnail();
    return;
  }

  IDataWriter *dw = nullptr;
  dwf->CreateDataWriter(out, &dw);
  dwf->Release();
  out->Release();
  if (!dw) {
    ra_stream->Release();
    ClearThumbnail();
    return;
  }

  dw->WriteBytes(static_cast<UINT32>(jpeg.size()), const_cast<BYTE *>(jpeg.data()));
  using StoreOp = __FIAsyncOperation_1_UINT32_t;
  using StoreHandler = __FIAsyncOperationCompletedHandler_1_UINT32_t;
  StoreOp *store_op = nullptr;
  dw->StoreAsync(&store_op);
  if (store_op) {
    HANDLE ev = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (ev) {
      auto cb = Microsoft::WRL::Callback<StoreHandler>([ev](StoreOp *, AsyncStatus) -> HRESULT {
        SetEvent(ev);
        return S_OK;
      });
      store_op->put_Completed(cb.Get());
      WaitForSingleObject(ev, 5000);
      CloseHandle(ev);
    }
    store_op->Release();
  }

  IOutputStream *detached = nullptr;
  dw->DetachStream(&detached);
  if (detached) {
    detached->Release();
  }
  dw->Release();

  IRandomAccessStreamReference *stream_ref = nullptr;
  {
    HSTRING h = nullptr;
    static const wchar_t kSrClass[] = L"Windows.Storage.Streams.RandomAccessStreamReference";
    WindowsCreateString(kSrClass, static_cast<UINT32>(wcslen(kSrClass)), &h);
    IRandomAccessStreamReferenceStatics *statics = nullptr;
    RoGetActivationFactory(h, __uuidof(IRandomAccessStreamReferenceStatics), reinterpret_cast<void **>(&statics));
    WindowsDeleteString(h);
    if (statics) {
      statics->CreateFromStream(ra_stream, &stream_ref);
      statics->Release();
    }
  }
  ra_stream->Release();
  if (!stream_ref) {
    ClearThumbnail();
    return;
  }
  updater->put_Thumbnail(stream_ref);
  stream_ref->Release();
  updater->Update();
#else
  (void)image;
#endif
}

void WinSystemMediaTransportControls::ClearThumbnail() {
#if defined(_WIN32) && defined(_MSC_VER)
  if (!updater_) {
    return;
  }
  ISystemMediaTransportControlsDisplayUpdater *updater = static_cast<ISystemMediaTransportControlsDisplayUpdater *>(updater_);
  updater->put_Thumbnail(nullptr);
  updater->Update();
#endif
}
