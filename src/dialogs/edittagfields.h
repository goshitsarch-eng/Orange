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

}  // namespace EditTagFields

#endif
