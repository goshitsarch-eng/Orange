#ifndef STRAWBERRY_MAINWINDOWSETTINGS_H
#define STRAWBERRY_MAINWINDOWSETTINGS_H

namespace MainWindowSettings {

constexpr char kSettingsGroup[] = "MainWindow";
constexpr char kSearchForCoverAuto[] = "search_for_cover_auto";
constexpr char kShowSidebar[] = "show_sidebar";
constexpr char kWidth[] = "width";
constexpr char kHeight[] = "height";
constexpr char kMaximized[] = "maximized";
constexpr char kMinimized[] = "minimized";
constexpr char kHidden[] = "hidden";
constexpr char kGeometry[] = "geometry";
constexpr char kSplitterState[] = "splitter_state";
constexpr char kDoNotShowSponsorMessage[] = "do_not_show_sponsor_message";
constexpr char kFilePath[] = "file_path";
constexpr char kIgnoreRosetta[] = "ignore_rosetta";
constexpr char kAskedPermission[] = "asked_permission";
constexpr char kAddMediaPath[] = "add_media_path";
constexpr char kAddFolderPath[] = "add_folder_path";

constexpr bool kDefaultSearchForCoverAuto = true;
constexpr bool kDefaultShowSidebar = true;
constexpr bool kDefaultMaximized = true;
constexpr bool kDefaultMinimized = false;
constexpr bool kDefaultHidden = false;
constexpr bool kDefaultDoNotShowSponsorMessage = false;

}  // namespace MainWindowSettings

#endif
