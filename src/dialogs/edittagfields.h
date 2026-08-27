#ifndef STRAWBERRY_EDITTAGFIELDS_H
#define STRAWBERRY_EDITTAGFIELDS_H

#include "core/song.h"

#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace EditTagFields {

std::pair<std::string, bool> CommonValue(const SongList &songs, const std::function<std::string(const Song &)> &getter);
void ApplyField(Song *song, const std::string &name, const std::string &value);
void ApplyChangedFields(SongList *songs, const std::vector<std::pair<std::string, std::string>> &changed);
void ResetPlayStatistics(Song *song);
void ResetPlayStatistics(SongList *songs);
int WrapIndex(int current, int delta, int count);
std::string SongRowLabel(const Song &song);

inline bool IsNumericIntField(const std::string &name) {
  return name == "Year" || name == "Original year" || name == "Track" || name == "Disc";
}

// Unset numeric tags are stored as -1 and shown as an empty field (Qt spinboxes show 0).
inline int ParseOriginalInt(const std::string &text) { return text.empty() ? -1 : std::atoi(text.c_str()); }

inline int ParseDisplayInt(const std::string &text) { return text.empty() ? 0 : std::atoi(text.c_str()); }

// Qt EditTagDialog::IsValueModified for track/disc/year: ignore unset (-1) vs displayed 0.
inline bool IsIntModified(int original, int current) { return original != current && (original != -1 || current != 0); }

inline bool IsRatingModified(double original, double current) { return original != current && (original != -1.0 || current != 0.0); }

inline bool IsValueModified(const std::string &name, const std::string &original, const std::string &current) {
  if (IsNumericIntField(name)) {
    return IsIntModified(ParseOriginalInt(original), ParseDisplayInt(current));
  }
  return original != current;
}

inline void NormalizeUnsetNumeric(Song *song) {
  if (!song) {
    return;
  }
  if (song->track() <= 0) {
    song->set_track(-1);
  }
  if (song->disc() <= 0) {
    song->set_disc(-1);
  }
  if (song->year() <= 0) {
    song->set_year(-1);
  }
  if (song->originalyear() <= 0) {
    song->set_originalyear(-1);
  }
  if (song->lastplayed() <= 0) {
    song->set_lastplayed(-1);
  }
}

inline double CommonRating(const SongList &songs) {
  if (songs.empty()) {
    return -1.0;
  }
  const float rating = songs.front().rating();
  for (const Song &song : songs) {
    if (song.rating() != rating) {
      return -1.0;
    }
  }
  return rating;
}

inline double RatingSliderFromStored(double stored) { return stored >= 0 ? stored * 5.0 : 0.0; }

inline float RatingStoredFromSlider(double slider) { return static_cast<float>(slider / 5.0); }

inline bool FetchTagsEnabled(bool have_tagfetcher, bool loading) { return have_tagfetcher && !loading; }

inline bool FieldsEnabled(bool loading, bool has_valid_songs) { return !loading && has_valid_songs; }

inline bool ButtonsEnabled(bool loading) { return !loading; }

inline bool SongListVisible(size_t count) { return count > 1; }

inline bool SongListEnabled(bool loading, bool has_valid_songs) { return !loading && has_valid_songs; }

inline bool SongListNavEnabled(bool list_visible, bool loading) { return list_visible && !loading; }

inline bool LoadingLabelVisible(bool loading) { return loading; }

inline const char *LoadingTracksMessage() { return "Loading tracks..."; }

inline const char *SavingTracksMessage() { return "Saving tracks..."; }

inline SongList ValidSongs(const SongList &songs) {
  SongList valid;
  for (const Song &song : songs) {
    if (song.IsEditable()) {
      valid.push_back(song);
    }
  }
  return valid;
}

inline bool AnySupported(const SongList &songs, bool (Song::*supported)() const) {
  for (const Song &song : songs) {
    if ((song.*supported)()) {
      return true;
    }
  }
  return false;
}

// Qt EditTagDialog::SelectionChanged: optional fields enable if any selected song can write them.
inline bool FieldEnabled(const std::string &name, const SongList &songs) {
  if (name == "Album artist") {
    return AnySupported(songs, &Song::albumartist_supported);
  }
  if (name == "Album artist sort") {
    return AnySupported(songs, &Song::albumartistsort_supported);
  }
  if (name == "Composer") {
    return AnySupported(songs, &Song::composer_supported);
  }
  if (name == "Composer sort") {
    return AnySupported(songs, &Song::composersort_supported);
  }
  if (name == "Performer") {
    return AnySupported(songs, &Song::performer_supported);
  }
  if (name == "Performer sort") {
    return AnySupported(songs, &Song::performersort_supported);
  }
  if (name == "Grouping") {
    return AnySupported(songs, &Song::grouping_supported);
  }
  if (name == "Genre") {
    return AnySupported(songs, &Song::genre_supported);
  }
  if (name == "Compilation") {
    return AnySupported(songs, &Song::compilation_supported);
  }
  if (name == "Rating") {
    return AnySupported(songs, &Song::rating_supported);
  }
  if (name == "Comment") {
    return AnySupported(songs, &Song::comment_supported);
  }
  if (name == "Lyrics") {
    return AnySupported(songs, &Song::lyrics_supported);
  }
  if (name == "Title sort") {
    return AnySupported(songs, &Song::titlesort_supported);
  }
  if (name == "Artist sort") {
    return AnySupported(songs, &Song::artistsort_supported);
  }
  if (name == "Album sort") {
    return AnySupported(songs, &Song::albumsort_supported);
  }
  return true;
}

}  // namespace EditTagFields

#endif
