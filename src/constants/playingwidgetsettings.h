#ifndef PLAYINGWIDGETSETTINGS_H
#define PLAYINGWIDGETSETTINGS_H

namespace PlayingWidgetSettings {

constexpr char kSettingsGroup[] = "PlayingWidget";
constexpr char kMode[] = "mode";
constexpr char kAboveStatusBar[] = "above_status_bar";
constexpr char kFitCoverWidth[] = "fit_cover_width";

constexpr int kDefaultMode = 1;
constexpr bool kDefaultAboveStatusBar = false;
constexpr bool kDefaultFitCoverWidth = false;

}  // namespace PlayingWidgetSettings

#endif
