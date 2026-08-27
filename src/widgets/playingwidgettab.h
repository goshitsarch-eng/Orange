#ifndef STRAWBERRY_PLAYINGWIDGETTAB_H
#define STRAWBERRY_PLAYINGWIDGETTAB_H

#include <cstring>

namespace PlayingWidgetTab {

inline const char *ContextTabId() { return "context"; }

inline bool OnContextTab(const char *name) { return name && std::strcmp(name, ContextTabId()) == 0; }

// Qt MainWindow::TabSwitched enables the playing widget when the behaviour pref is on, the sidebar is visible, and we are not on Context with the album pane shown.
inline bool ShouldEnable(bool pref, bool sidebar_visible, bool on_context_tab, bool album_enabled) {
  return pref && sidebar_visible && (!on_context_tab || !album_enabled);
}

inline bool ShouldRefreshOnTabChange() { return true; }

inline bool ShouldRefreshOnSidebarToggle() { return true; }

inline bool ShouldRefreshOnAlbumEnabled() { return true; }

}  // namespace PlayingWidgetTab

#endif
