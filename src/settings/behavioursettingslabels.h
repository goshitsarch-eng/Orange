#ifndef STRAWBERRY_BEHAVIOURSETTINGSLABELS_H
#define STRAWBERRY_BEHAVIOURSETTINGSLABELS_H

#include <string>
#include <utility>
#include <vector>

namespace BehaviourSettingsLabels {

inline const char *PageTitle() { return "Behavior"; }
inline const char *ShowTray() { return "Show system tray icon"; }
inline const char *KeepRunning() { return "Keep running in the background when the window is closed"; }
inline const char *TrayProgress() { return "Show song progress on system tray icon"; }
inline const char *TaskbarProgress() { return "Show song progress on taskbar"; }
inline const char *ResumePlayback() { return "Resume playback on start"; }
inline const char *PlayingWidget() { return "Show playing widget"; }
inline const char *OnStartup() { return "On startup"; }
inline const char *Remember() { return "Remember from last time"; }
inline const char *ShowWindow() { return "Show the main window"; }
inline const char *HideWindow() { return "Hide the main window"; }
inline const char *ShowMaximized() { return "Show the main window maximized"; }
inline const char *ShowMinimized() { return "Show the main window minimized"; }
inline const char *Language() { return "Language"; }
inline const char *SystemLanguage() { return "Use the system default"; }
inline const char *LanguageRestart() { return "You will need to restart Strawberry if you change the language."; }
inline const char *MenuPlay() { return "Using the menu to add a song will..."; }
inline const char *NeverPlay() { return "Never start playing"; }
inline const char *PlayIfStopped() { return "Play if there is nothing already playing"; }
inline const char *AlwaysPlay() { return "Always start playing"; }
inline const char *PreviousMode() { return "Pressing \"Previous\" in player will..."; }
inline const char *PreviousJump() { return "Jump to previous song right away"; }
inline const char *PreviousRestart() { return "Restart song, then jump to previous if pressed again"; }
inline const char *DoubleClickAdd() { return "Double clicking a song will..."; }
inline const char *Append() { return "Append to the playlist"; }
inline const char *Replace() { return "Replace the playlist"; }
inline const char *OpenInNew() { return "Open in new playlist"; }
inline const char *Enqueue() { return "Add to the queue"; }
inline const char *DoubleClickPlay() { return "Double clicking a song will..."; }
inline const char *DoubleClickPlaylist() { return "Double clicking a song in the playlist will..."; }
inline const char *ChangePlaying() { return "Change the currently playing song"; }
inline const char *Seeking() { return "Seeking using a keyboard shortcut or mouse wheel"; }
inline const char *TimeStep() { return "Time step"; }
inline const char *SecondsSuffix() { return " s"; }
inline const char *VolumeIncrement() { return "Volume Increment"; }

inline std::vector<std::pair<std::string, std::string>> PlayChoices() {
  return {{"1", NeverPlay()}, {"2", PlayIfStopped()}, {"3", AlwaysPlay()}};
}

inline std::vector<std::pair<std::string, std::string>> PreviousChoices() {
  return {{"1", PreviousJump()}, {"2", PreviousRestart()}};
}

inline std::vector<std::pair<std::string, std::string>> DoubleClickAddChoices() {
  return {{"1", Append()}, {"3", Replace()}, {"4", OpenInNew()}, {"2", Enqueue()}};
}

inline std::vector<std::pair<std::string, std::string>> DoubleClickPlaylistChoices() {
  return {{"1", ChangePlaying()}, {"2", Enqueue()}};
}

}  // namespace BehaviourSettingsLabels

#endif
