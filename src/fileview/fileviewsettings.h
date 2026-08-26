#ifndef STRAWBERRY_FILEVIEWSETTINGS_H
#define STRAWBERRY_FILEVIEWSETTINGS_H

namespace FileViewSettings {

constexpr char kSettingsGroup[] = "FileView";
constexpr char kShowHidden[] = "show_hidden";
constexpr char kShowAllFiles[] = "show_all_files";
constexpr char kTreeViewActive[] = "tree_view_active";
constexpr char kTreeRootPaths[] = "tree_root_paths";
constexpr bool kDefaultShowHidden = false;
constexpr bool kDefaultShowAllFiles = false;
constexpr bool kDefaultTreeViewActive = false;

}  // namespace FileViewSettings

#endif
