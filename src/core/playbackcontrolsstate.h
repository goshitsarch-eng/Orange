#ifndef STRAWBERRY_PLAYBACKCONTROLSSTATE_H
#define STRAWBERRY_PLAYBACKCONTROLSSTATE_H

namespace PlaybackControlsState {

// Qt MainWindow::MediaPlaying enables Play/Pause only when the item is not PauseDisabled.
// SetPaused and SetStopped always leave Play/Pause enabled.
inline bool PlayPauseEnabled(bool playing, bool pause_disabled) { return !playing || !pause_disabled; }

// Qt MediaStopped / SetPlaying / SetPaused: Stop and Stop after are enabled only while playing or paused.
inline bool PlaybackActive(bool playing, bool paused) { return playing || paused; }

inline bool StopEnabled(bool playback_active) { return playback_active; }

}  // namespace PlaybackControlsState

#endif
