#ifndef STRAWBERRY_STYLEUTILS_H
#define STRAWBERRY_STYLEUTILS_H

#include <gtk/gtk.h>
#include <string>

namespace StyleUtils {

// Style slots.
// Each slot owns exactly one GtkCssProvider that stays attached to the display for the lifetime of the
// application.
// Re-loading a slot replaces the CSS in the provider it already owns instead of attaching another one,
// so repeatedly restyling (the playlist glow animation ticks ~16 times a second) neither leaks providers
// nor makes every subsequent style lookup walk a longer and longer provider chain.
namespace Slot {
inline constexpr char kAppearanceStyle[] = "appearance-style";
inline constexpr char kAppearanceTheme[] = "appearance-theme";
inline constexpr char kContextHeadline[] = "context-headline";
inline constexpr char kContextIdle[] = "context-idle";
inline constexpr char kContextNormal[] = "context-normal";
inline constexpr char kOsdPrettyChrome[] = "osd-pretty-chrome";
inline constexpr char kOsdPrettyMetrics[] = "osd-pretty-metrics";
inline constexpr char kPlaylistBackground[] = "playlist-background";
inline constexpr char kPlaylistLook[] = "playlist-look";
inline constexpr char kQueueLook[] = "queue-look";
inline constexpr char kUserStyleSheet[] = "user-stylesheet";
}  // namespace Slot

// Replaces the CSS held by the named slot, attaching the slot's provider to the default display the first
// time the slot is used.
// GTK re-styles automatically when a provider that is already attached is re-loaded.
void LoadCss(const std::string &css, const std::string &slot);

// Drops the CSS held by the named slot without detaching its provider.
void ClearCss(const std::string &slot);

bool IsDarkTheme();

}  // namespace StyleUtils

#endif
