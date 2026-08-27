#ifndef STRAWBERRY_EDITTAGCOVER_H
#define STRAWBERRY_EDITTAGCOVER_H

#include "core/song.h"
#include "covermanager/coveroptions.h"

namespace EditTagCover {

inline bool AnySupported(const SongList &songs) {
  for (const Song &song : songs) {
    if (song.save_embedded_cover_supported()) {
      return true;
    }
  }
  return false;
}

inline bool DefaultEmbeddedChecked(const Song &song, CoverOptions::CoverType collection_save_type) {
  return (song.art_embedded() || collection_save_type == CoverOptions::CoverType::Embedded) && song.save_embedded_cover_supported();
}

inline CoverOptions::CoverType EffectiveSaveType(CoverOptions::CoverType collection_save_type, bool embedded_override) {
  return embedded_override ? CoverOptions::CoverType::Embedded : collection_save_type;
}

inline std::string ImageBytes(const std::string &image_or_path) {
  if (FileUtils::Exists(image_or_path) && FileUtils::IsFile(image_or_path)) {
    return FileUtils::ReadFile(image_or_path);
  }
  return image_or_path;
}

// Qt EditTagDialog::is_local_collection_song — cover changes apply to collection albums only.
inline bool ChangeArtEnabled(const Song &song) { return song.is_collection_song(); }

// After a cover load finishes, Qt also requires album identity before changing art.
inline bool ChangeArtEnabledWithAlbum(const Song &song) {
  return song.is_collection_song() && !song.EffectiveAlbumartist().empty() && !song.album().empty();
}

inline bool HasValidArt(const Song &song) {
  return song.art_embedded() || !song.art_automatic().empty() || !song.art_manual().empty();
}

inline bool ShowCoverEnabled(const Song &song, bool art_different) {
  return !art_different && HasValidArt(song) && !song.art_unset();
}

// Qt EditTagDialog::eventFilter: double-click summary_art / tags_art calls ShowCover.
inline constexpr int kPrimaryButton = 1;
inline constexpr int kSummaryArtSize = 160;
inline constexpr int kTagsArtSize = 128;

inline const char *ChangeArt() { return "Change art"; }

inline bool ShowOnDoubleClick(const Song &song, bool art_different) { return ShowCoverEnabled(song, art_different); }

inline bool IsDoubleClick(int n_press, unsigned button) { return n_press >= 2 && static_cast<int>(button) == kPrimaryButton; }

inline bool IsMenuClick(int n_press, unsigned button) { return n_press == 1 && static_cast<int>(button) == kPrimaryButton; }

inline bool SaveCoverEnabled(const Song &song, bool art_different) { return ShowCoverEnabled(song, art_different); }

inline bool FromFileEnabled(bool change_art) { return change_art; }

inline bool FromUrlEnabled(bool change_art) { return change_art; }

inline bool SearchCoverEnabled(bool has_providers, bool change_art, bool art_different) {
  return change_art && (art_different || has_providers);
}

inline bool FetchCoverEnabled(bool change_art) { return change_art; }

inline bool UnsetCoverEnabled(const Song &song, bool change_art, bool art_different) {
  return change_art && (art_different || !song.art_unset());
}

inline bool ClearCoverEnabled(const Song &song, bool change_art, bool art_different) {
  return change_art && (art_different || !song.art_manual().empty() || song.art_unset());
}

inline bool DeleteCoverEnabled(const Song &song, bool change_art, bool art_different) {
  return change_art && (art_different || song.art_embedded() || !song.art_automatic().empty() || !song.art_manual().empty());
}

inline const char *ArtDifferentHint() { return "Different art across multiple songs."; }

inline const char *TagsDifferentHint() { return "(different across multiple songs)"; }

inline bool ArtDifferent(const Song &first, const Song &other) {
  return other.art_manual() != first.art_manual() || other.art_embedded() != first.art_embedded() ||
         other.art_automatic() != first.art_automatic() ||
         (other.art_embedded() && first.art_embedded() &&
          (first.EffectiveAlbumartist() != other.EffectiveAlbumartist() || first.album() != other.album()));
}

inline bool ArtDifferentAcrossSongs(const SongList &songs) {
  if (songs.size() < 2) {
    return false;
  }
  const Song &first = songs.front();
  for (size_t i = 1; i < songs.size(); ++i) {
    if (ArtDifferent(first, songs[i])) {
      return true;
    }
  }
  return false;
}

inline bool SummaryTabEnabled(size_t selected_count) { return selected_count <= 1; }

inline bool LyricsTabEnabled(size_t selected_count) { return selected_count <= 1; }

// Qt edittagdialog.ui: rating, title/artist/album/year/track/genre, and embedded cover live on tab_tags.
inline bool RatingOnTagsTab() { return true; }

inline bool CoreFieldsOnTagsTab() { return true; }

inline bool EmbeddedCoverOnTagsTab() { return true; }

}  // namespace EditTagCover

#endif
