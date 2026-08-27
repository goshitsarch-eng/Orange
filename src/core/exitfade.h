#ifndef STRAWBERRY_EXITFADE_H
#define STRAWBERRY_EXITFADE_H

namespace ExitFade {

// Qt MainWindow::Exit: wait for EngineBase::Finished when fade-out is enabled
// and a track is playing. A second quit skips the rest of the fade. A third
// quit while shutdown is already running aborts without tearing down threads.
enum class Action { ShutdownNow, WaitForFade, SkipFade, AbortProcess };

inline Action Decide(int exit_count, bool fadeout_enabled, bool playing, bool exit_started) {
  if (exit_count > 1) {
    return exit_started ? Action::AbortProcess : Action::SkipFade;
  }
  if (fadeout_enabled && playing) {
    return Action::WaitForFade;
  }
  return Action::ShutdownNow;
}

inline bool ShouldHideUi(Action action) { return action == Action::WaitForFade; }

inline bool ShouldKeepWindow(Action action) { return action == Action::WaitForFade; }

}  // namespace ExitFade

#endif
