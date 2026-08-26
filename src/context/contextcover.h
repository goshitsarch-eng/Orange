#ifndef STRAWBERRY_CONTEXTCOVER_H
#define STRAWBERRY_CONTEXTCOVER_H

#include "constants/contextsettings.h"
#include "core/mainwindowsettings.h"
#include "core/settings.h"

#include <string>

namespace ContextCover {

inline std::string EffectiveAlbum(const std::string &album, const std::string &title) { return album.empty() ? title : album; }

inline bool ArtPathLooksValid(const std::string &path) { return !path.empty(); }

inline bool HasExistingCover(bool art_embedded, bool art_automatic_valid, bool art_manual_valid) {
  return art_embedded || art_automatic_valid || art_manual_valid;
}

inline bool HasAlbumIdentity(const std::string &albumartist, const std::string &album) {
  return !albumartist.empty() && !album.empty();
}

inline bool ShouldSearch(bool context_enabled, bool covers_automatic, bool art_unset, bool already_has_cover) {
  return context_enabled && covers_automatic && !art_unset && !already_has_cover;
}

inline bool ShouldSearchForSong(bool context_enabled, bool covers_automatic, bool art_unset, bool art_embedded,
                                const std::string &art_automatic, const std::string &art_manual, const std::string &albumartist,
                                const std::string &album) {
  return ShouldSearch(context_enabled, covers_automatic, art_unset,
                      HasExistingCover(art_embedded, ArtPathLooksValid(art_automatic), ArtPathLooksValid(art_manual))) &&
         HasAlbumIdentity(albumartist, album);
}

inline bool LoadEnabled(Settings &settings) {
  settings.BeginGroup(MainWindowSettings::kSettingsGroup);
  if (settings.Contains(MainWindowSettings::kSearchForCoverAuto)) {
    return settings.BoolValue(MainWindowSettings::kSearchForCoverAuto, MainWindowSettings::kDefaultSearchForCoverAuto);
  }
  settings.BeginGroup(ContextSettings::kSettingsGroup);
  return settings.BoolValue(ContextSettings::kSearchCover, ContextSettings::kDefaultSearchCover);
}

inline void PersistEnabled(Settings &settings, bool enabled) {
  settings.BeginGroup(ContextSettings::kSettingsGroup);
  settings.SetBoolValue(ContextSettings::kSearchCover, enabled);
  settings.BeginGroup(MainWindowSettings::kSettingsGroup);
  settings.SetBoolValue(MainWindowSettings::kSearchForCoverAuto, enabled);
  settings.Sync();
  settings.BeginGroup(ContextSettings::kSettingsGroup);
}

}  // namespace ContextCover

#endif
