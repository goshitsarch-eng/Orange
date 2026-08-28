#ifndef STRAWBERRY_TASKBARPROGRESS_H
#define STRAWBERRY_TASKBARPROGRESS_H

#include <algorithm>
#include <cstdint>
#include <string>

#include <gio/gio.h>

namespace TaskbarProgressHelpers {

inline double Fraction(int64_t position, int64_t length) {
  if (length <= 0) {
    return 0.0;
  }
  const double value = static_cast<double>(position) / static_cast<double>(length);
  return std::clamp(value, 0.0, 1.0);
}

inline bool ShouldShow(bool enabled, bool playing, int64_t length) { return enabled && playing && length > 0; }

// Qt MainWindow::ReloadSettings clears Unity progress immediately when the pref is turned off.
inline bool ShouldClearImmediately(bool currently_visible, bool now_enabled) { return currently_visible && !now_enabled; }

inline const char *AppUri() { return "application://io.github.goshitsarch_eng.Orange.desktop"; }

inline const char *Interface() { return "com.canonical.Unity.LauncherEntry"; }

inline std::string ObjectPath() { return "/com/canonical/unity/launcherentry/strawberry"; }

}  // namespace TaskbarProgressHelpers

class TaskbarProgress {
 public:
  TaskbarProgress();
  ~TaskbarProgress();

  void Set(double fraction, bool visible);
  double fraction() const { return fraction_; }
  bool visible() const { return visible_; }

 private:
  void EnsureConnection();
  void Emit();

  GDBusConnection *connection_ = nullptr;
  double fraction_ = 0.0;
  bool visible_ = false;
  bool connected_ = false;
};

#endif
