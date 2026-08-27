#ifndef STRAWBERRY_COVERSSETTINGSLABELS_H
#define STRAWBERRY_COVERSSETTINGSLABELS_H

#include <string>
#include <utility>
#include <vector>

namespace CoversSettingsLabels {

inline const char *ProvidersGroup() { return "Cover providers"; }
inline const char *ProvidersHint() { return "Choose the providers you want to use when searching for covers."; }
inline const char *TypesGroup() { return "Album cover types"; }
inline const char *SavingGroup() { return "Saving album covers"; }
inline const char *FilenameGroup() { return "Filename:"; }
inline const char *SaveAlbumDir() { return "Save album covers in album directory"; }
inline const char *SaveCache() { return "Save album covers in cache directory"; }
inline const char *SaveEmbedded() { return "Save album covers as embedded cover"; }
inline const char *FilenamePattern() { return "Pattern"; }
inline const char *FilenameRandom() { return "Random"; }
inline const char *Overwrite() { return "Overwrite existing file"; }
inline const char *Lowercase() { return "Lowercase filename"; }
inline const char *ReplaceSpaces() { return "Replace spaces with dashes"; }
inline const char *Authentication() { return "Authentication"; }
inline const char *AuthHint() { return "Streaming cover providers authenticate from their service settings pages."; }
inline const char *AutomaticSearch() { return "Automatically search for album cover"; }

inline std::vector<std::pair<std::string, std::string>> SaveTypeChoices() {
  return {
      {"2", SaveAlbumDir()},
      {"1", SaveCache()},
      {"3", SaveEmbedded()},
  };
}

inline std::vector<std::pair<std::string, std::string>> FilenameChoices() {
  return {
      {"2", FilenamePattern()},
      {"1", FilenameRandom()},
  };
}

inline const char *DefaultSaveType() { return "1"; }
inline const char *DefaultFilename() { return "2"; }

}  // namespace CoversSettingsLabels

#endif
