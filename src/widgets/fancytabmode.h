#ifndef STRAWBERRY_FANCYTABMODE_H
#define STRAWBERRY_FANCYTABMODE_H

#include <gtk/gtk.h>

#include <vector>

namespace FancyTabMode {

// Values match Qt FancyTabWidget::Mode so stored tab_mode integers stay compatible.
enum class Mode {
  None = 0,
  LargeSidebar = 1,
  SmallSidebar = 2,
  Tabs = 3,
  IconOnlyTabs = 4,
  PlainSidebar = 5,
  IconsSidebar = 6,
};

inline constexpr const char *kTabMode = "tab_mode";
inline constexpr const char *kCurrentTab = "current_tab";
inline constexpr Mode kDefaultMode = Mode::LargeSidebar;
inline constexpr int kDefaultCurrentTab = 1;
inline constexpr int kLargeIcon = 40;
inline constexpr int kSmallIcon = 32;
// Caps how wide a single tab title may make the sidebar before it wraps onto another line.
inline constexpr int kSidebarLabelMaxChars = 14;

struct Item {
  const char *label = "";
  Mode mode = Mode::LargeSidebar;
};

inline std::vector<Item> MenuItems() {
  return {
      {"Large sidebar", Mode::LargeSidebar},
      {"Icons sidebar", Mode::IconsSidebar},
      {"Small sidebar", Mode::SmallSidebar},
      {"Plain sidebar", Mode::PlainSidebar},
      {"Tabs on top", Mode::Tabs},
      {"Icons on top", Mode::IconOnlyTabs},
  };
}

inline int ItemCount() { return static_cast<int>(MenuItems().size()); }

inline Mode FromStored(int raw) {
  if (raw < static_cast<int>(Mode::LargeSidebar) || raw > static_cast<int>(Mode::IconsSidebar)) {
    return kDefaultMode;
  }
  return static_cast<Mode>(raw);
}

inline int ToStored(Mode mode) { return static_cast<int>(mode == Mode::None ? kDefaultMode : mode); }

inline bool IsTop(Mode mode) { return mode == Mode::Tabs || mode == Mode::IconOnlyTabs; }

inline bool ShowsText(Mode mode) { return mode != Mode::IconOnlyTabs && mode != Mode::IconsSidebar; }

inline bool ShowsIcon(Mode mode) { return mode != Mode::PlainSidebar; }

inline bool IsLargeIcon(Mode mode) { return mode == Mode::LargeSidebar || mode == Mode::IconsSidebar; }

// How the buttons are laid out inside the tab bar itself.
inline GtkOrientation BarOrientation(Mode mode) { return IsTop(mode) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL; }

// How the tab bar and the stack are laid out relative to each other, which is the opposite axis: a vertical
// bar sits beside the stack, a horizontal bar sits above it.
inline GtkOrientation PanelOrientation(Mode mode) { return IsTop(mode) ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL; }

inline GtkOrientation ButtonOrientation(Mode mode) {
  return mode == Mode::LargeSidebar || mode == Mode::IconsSidebar ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
}

inline int IconSize(Mode mode, int large, int small) { return IsLargeIcon(mode) ? large : small; }

inline bool StackExpandsBesideBar(Mode mode) { return !IsTop(mode); }

// Qt FancyTabWidget::contextMenuEvent always pops when the event is on the tab bar.
constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

inline bool ShouldShowMenu() { return true; }

}  // namespace FancyTabMode

#endif
