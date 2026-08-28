#ifndef STRAWBERRY_WINDOWGEOMETRY_H
#define STRAWBERRY_WINDOWGEOMETRY_H

namespace WindowGeometry {

constexpr char kSettingsGroup[] = "MainWindow";
constexpr char kWidth[] = "width";
constexpr char kHeight[] = "height";
constexpr char kMaximized[] = "maximized";
constexpr int kDefaultWidth = 1400;
constexpr int kDefaultHeight = 860;
constexpr int kMinWidth = 640;
constexpr int kMinHeight = 480;
constexpr int kMaxWidth = 8192;
constexpr int kMaxHeight = 8192;

struct State {
  int width = kDefaultWidth;
  int height = kDefaultHeight;
  bool maximized = false;
};

State Clamp(State state);
State FromValues(int width, int height, bool maximized);
int StartupAction(int startup_behaviour, bool remembered_maximized);

// Qt MainWindow::hideEvent remembers maximized/minimized so tray restore can match.
enum class AfterHide { Show, Maximize, Minimize };

inline AfterHide RestoreAfterHide(bool was_maximized, bool was_minimized) {
  if (was_minimized) {
    return AfterHide::Minimize;
  }
  if (was_maximized) {
    return AfterHide::Maximize;
  }
  return AfterHide::Show;
}

// Qt Hide falls through to Remember when the tray is missing. Remember restores hidden only when the tray can show the window.
enum class StartupShow { Show, Maximize, Minimize, Hide };

inline bool HideFallsThroughToRemember(bool tray_available_and_visible) { return !tray_available_and_visible; }

inline StartupShow RememberShow(bool maximized, bool minimized, bool hidden, bool can_hide) {
  if (hidden && can_hide) {
    return StartupShow::Hide;
  }
  if (minimized) {
    return StartupShow::Minimize;
  }
  if (maximized) {
    return StartupShow::Maximize;
  }
  return StartupShow::Show;
}

inline StartupShow ResolveStartup(int startup_behaviour, bool remembered_maximized, bool remembered_minimized, bool remembered_hidden,
                                  bool tray_available_and_visible) {
  switch (startup_behaviour) {
    case 3:
      return StartupShow::Hide;
    case 4:
      return StartupShow::Maximize;
    case 5:
      return StartupShow::Minimize;
    case 2:
      return StartupShow::Show;
    case 1:
    default:
      return RememberShow(remembered_maximized, remembered_minimized, remembered_hidden, tray_available_and_visible);
  }
}

}  // namespace WindowGeometry

#endif
