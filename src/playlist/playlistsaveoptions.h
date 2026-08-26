#ifndef STRAWBERRY_PLAYLISTSAVEOPTIONS_H
#define STRAWBERRY_PLAYLISTSAVEOPTIONS_H

#include "constants/playlistsettings.h"
#include "core/settings.h"

namespace PlaylistSaveOptions {

inline const char *Title() { return "Playlist options"; }
inline const char *PathsLabel() { return "File paths"; }
inline const char *RememberLabel() { return "Remember my choice"; }
inline const char *Hint() { return "This can be changed later through the preferences"; }
inline const char *SettingsPrompt() { return "When saving a playlist, file paths should be"; }
inline const char *AskWhenSaving() { return "Ask when saving"; }

inline constexpr PlaylistSettings::PathType kDialogChoices[] = {
    PlaylistSettings::PathType::Automatic, PlaylistSettings::PathType::Relative, PlaylistSettings::PathType::Absolute};

inline constexpr int kDialogChoiceCount = 3;

inline const char *DialogLabel(PlaylistSettings::PathType type) {
  switch (type) {
    case PlaylistSettings::PathType::Relative:
      return "Relative";
    case PlaylistSettings::PathType::Absolute:
      return "Absolute";
    case PlaylistSettings::PathType::Automatic:
    default:
      return "Automatic";
  }
}

inline PlaylistSettings::PathType PathFromIndex(int index) {
  if (index < 0 || index >= kDialogChoiceCount) {
    return PlaylistSettings::PathType::Automatic;
  }
  return kDialogChoices[index];
}

inline int IndexFromPath(PlaylistSettings::PathType type) {
  for (int i = 0; i < kDialogChoiceCount; ++i) {
    if (kDialogChoices[i] == type) {
      return i;
    }
  }
  return 0;
}

inline bool ShouldPersist(bool remember) { return remember; }

inline void MaybeRemember(bool remember, PlaylistSettings::PathType type) {
  if (!remember) {
    return;
  }
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  settings.SetIntValue(PlaylistSettings::kPathType, static_cast<int>(type));
  settings.Sync();
}

}  // namespace PlaylistSaveOptions

#endif  // STRAWBERRY_PLAYLISTSAVEOPTIONS_H
