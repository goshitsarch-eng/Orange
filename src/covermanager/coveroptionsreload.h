#ifndef STRAWBERRY_COVEROPTIONSRELOAD_H
#define STRAWBERRY_COVEROPTIONSRELOAD_H

#include "covermanager/coveroptions.h"

namespace CoverOptionsReload {

// Qt MainWindow::ReloadAllSettings always calls AlbumCoverChoiceController::ReloadSettings.

inline bool ShouldReloadOnSettingsClose() { return true; }

inline bool OptionsDiffer(const CoverOptions &before, const CoverOptions &after) {
  return before.cover_type != after.cover_type || before.cover_filename != after.cover_filename ||
         before.cover_pattern != after.cover_pattern || before.cover_overwrite != after.cover_overwrite ||
         before.cover_lowercase != after.cover_lowercase || before.cover_replace_spaces != after.cover_replace_spaces;
}

}  // namespace CoverOptionsReload

#endif
