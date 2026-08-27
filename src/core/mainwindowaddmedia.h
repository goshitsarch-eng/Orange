#ifndef STRAWBERRY_MAINWINDOWADDMEDIA_H
#define STRAWBERRY_MAINWINDOWADDMEDIA_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"
#include "core/mainwindowsettings.h"
#include "utilities/fileutils.h"

#include <string>

namespace MainWindowAddMedia {

// Qt MainWindow::AddFile / AddFolder dialog titles.
inline const char *AddFileTitle() { return "Add file"; }
inline const char *AddFolderTitle() { return "Add folder"; }

// Qt Music "Add folder..." calls AddFolder (playlist), not collection settings.
inline bool FolderAddsToPlaylist() { return true; }

inline std::string InitialFolder(const std::string &stored, const std::string &fallback) {
  if (!stored.empty() && FileUtils::IsDirectory(stored)) {
    return stored;
  }
  if (!stored.empty()) {
    const std::string dir = FileUtils::DirName(stored);
    if (FileUtils::IsDirectory(dir)) {
      return dir;
    }
  }
  return fallback;
}

inline CollectionBehaviour::Plan MenuAppendPlan(BehaviourSettings::PlayBehaviour play, bool engine_stopped) {
  return CollectionBehaviour::Append(play, engine_stopped);
}

}  // namespace MainWindowAddMedia

#endif
