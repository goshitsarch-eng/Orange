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

}  // namespace WindowGeometry

#endif
