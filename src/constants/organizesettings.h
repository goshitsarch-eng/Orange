#ifndef ORGANIZESETTINGS_H
#define ORGANIZESETTINGS_H

namespace OrganizeSettings {

constexpr char kSettingsGroup[] = "Organize";
constexpr char kFormat[] = "format";
constexpr char kDestination[] = "destination";
constexpr char kMove[] = "move";
constexpr char kOverwrite[] = "overwrite";
constexpr char kReplaceSpaces[] = "replace_spaces";
constexpr char kRemoveProblematic[] = "remove_problematic";
constexpr char kRemoveNonFat[] = "remove_non_fat";
constexpr char kRemoveNonAscii[] = "remove_non_ascii";
constexpr char kAllowAsciiExt[] = "allow_ascii_ext";
constexpr char kAlbumCover[] = "albumcover";
constexpr char kEjectAfter[] = "eject_after";

constexpr char kDefaultFormat[] = "%albumartist/%album/{%track - }%title";
constexpr bool kDefaultMove = false;
constexpr bool kDefaultOverwrite = false;
constexpr bool kDefaultReplaceSpaces = true;
constexpr bool kDefaultRemoveProblematic = true;
constexpr bool kDefaultRemoveNonFat = false;
constexpr bool kDefaultRemoveNonAscii = false;
constexpr bool kDefaultAllowAsciiExt = false;
constexpr bool kDefaultAlbumCover = true;
constexpr bool kDefaultEjectAfter = false;

}  // namespace OrganizeSettings

#endif
