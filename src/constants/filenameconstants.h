#ifndef STRAWBERRY_FILENAMECONSTANTS_H
#define STRAWBERRY_FILENAMECONSTANTS_H

namespace FilenameConstants {
constexpr char kIllegal[] = "/\\:*?\"<>|";
constexpr char kProblematicCharacters[] = ":?*\"<>|";
constexpr char kFatAllowedPunctuation[] = "!#$%&'()-@^_`{}~/. ";
constexpr char kInvalidPrefixCharacters[] = ".";
constexpr char kDefaultCover[] = "cover";
}  // namespace FilenameConstants

#endif
