#ifndef STRAWBERRY_MAINWINDOWSETTINGS_H
#define STRAWBERRY_MAINWINDOWSETTINGS_H

namespace MainWindowSettings {
constexpr char kSettingsGroup[] = "MainWindow";
constexpr char kSearchForCoverAuto[] = "search_for_cover_auto";
constexpr char kWidth[] = "width";
constexpr char kHeight[] = "height";
constexpr char kMaximized[] = "maximized";

constexpr bool kDefaultSearchForCoverAuto = true;
}  // namespace MainWindowSettings

#endif
