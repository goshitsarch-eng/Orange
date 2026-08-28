#ifndef STRAWBERRY_SETTINGSDIALOGSHOW_H
#define STRAWBERRY_SETTINGSDIALOGSHOW_H

namespace SettingsDialogShow {

// Qt SettingsDialog::showEvent reloads geometry and every page Load() on a programmatic show.
inline bool ShouldLoadGeometry(bool spontaneous) { return !spontaneous; }

inline bool ShouldReloadPages(bool spontaneous) { return !spontaneous; }

}  // namespace SettingsDialogShow

#endif
