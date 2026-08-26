#ifndef STRAWBERRY_COLLECTIONDIVIDER_H
#define STRAWBERRY_COLLECTIONDIVIDER_H

#include "collection/collectiongrouping.h"
#include "collection/collectionitem.h"
#include "constants/collectionsettings.h"
#include "core/settings.h"
#include "core/song.h"

#include <glib.h>

#include <cstdlib>
#include <string>

namespace CollectionDivider {

inline bool LoadShowDividers() {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  return settings.BoolValue(CollectionSettings::kShowDividers, CollectionSettings::kDefaultShowDividers);
}

inline bool ShouldInsert(bool show_dividers, int container_level, const std::string &key) {
  return show_dividers && container_level == 0 && !key.empty();
}

inline bool IsDivider(const CollectionItem *item) { return item && item->type == CollectionItem::Type::Divider; }

inline std::string SortTextForNumber(int number) {
  char buf[16] = {};
  g_snprintf(buf, sizeof(buf), "%04d", number);
  return buf;
}

inline std::string SortTextForBitrate(int bitrate) {
  char buf[16] = {};
  g_snprintf(buf, sizeof(buf), "%03d", bitrate);
  return buf;
}

inline std::string LetterKey(const std::string &sort_text) {
  if (sort_text.empty()) {
    return {};
  }
  gchar *norm = g_utf8_normalize(sort_text.c_str(), -1, G_NORMALIZE_NFD);
  const char *use = norm ? norm : sort_text.c_str();
  const gunichar c = g_utf8_get_char(use);
  if (norm) {
    g_free(norm);
  }
  if (c == ' ' || c == 0) {
    return {};
  }
  if (g_unichar_isdigit(c)) {
    return "0";
  }
  gchar out[8] = {};
  g_unichar_to_utf8(g_unichar_toupper(c), out);
  return out;
}

inline std::string Key(CollectionGrouping::GroupBy group_by, const Song &song, const std::string &sort_text) {
  if (sort_text.empty()) {
    return {};
  }
  switch (group_by) {
    case CollectionGrouping::GroupBy::AlbumArtist:
    case CollectionGrouping::GroupBy::Artist:
    case CollectionGrouping::GroupBy::Album:
    case CollectionGrouping::GroupBy::AlbumDisc:
    case CollectionGrouping::GroupBy::Composer:
    case CollectionGrouping::GroupBy::Performer:
    case CollectionGrouping::GroupBy::Grouping:
    case CollectionGrouping::GroupBy::Disc:
    case CollectionGrouping::GroupBy::Genre:
    case CollectionGrouping::GroupBy::Format:
    case CollectionGrouping::GroupBy::FileType:
      return LetterKey(sort_text);
    case CollectionGrouping::GroupBy::Year:
    case CollectionGrouping::GroupBy::OriginalYear:
      return SortTextForNumber(std::atoi(sort_text.c_str()) / 10 * 10);
    case CollectionGrouping::GroupBy::YearAlbum:
    case CollectionGrouping::GroupBy::YearAlbumDisc:
      return SortTextForNumber(song.year());
    case CollectionGrouping::GroupBy::OriginalYearAlbum:
    case CollectionGrouping::GroupBy::OriginalYearAlbumDisc:
      return SortTextForNumber(CollectionGrouping::EffectiveOriginalYear(song));
    case CollectionGrouping::GroupBy::Samplerate:
      return SortTextForNumber(song.samplerate());
    case CollectionGrouping::GroupBy::Bitdepth:
      return SortTextForNumber(song.bitdepth());
    case CollectionGrouping::GroupBy::Bitrate:
      return SortTextForBitrate(song.bitrate());
    case CollectionGrouping::GroupBy::None:
    case CollectionGrouping::GroupBy::GroupByCount:
      return {};
  }
  return {};
}

inline std::string DisplayText(CollectionGrouping::GroupBy group_by, const std::string &key) {
  switch (group_by) {
    case CollectionGrouping::GroupBy::AlbumArtist:
    case CollectionGrouping::GroupBy::Artist:
    case CollectionGrouping::GroupBy::Album:
    case CollectionGrouping::GroupBy::AlbumDisc:
    case CollectionGrouping::GroupBy::Composer:
    case CollectionGrouping::GroupBy::Performer:
    case CollectionGrouping::GroupBy::Disc:
    case CollectionGrouping::GroupBy::Grouping:
    case CollectionGrouping::GroupBy::Genre:
    case CollectionGrouping::GroupBy::FileType:
    case CollectionGrouping::GroupBy::Format:
      return key == "0" ? "0-9" : key;
    case CollectionGrouping::GroupBy::YearAlbum:
    case CollectionGrouping::GroupBy::YearAlbumDisc:
    case CollectionGrouping::GroupBy::OriginalYearAlbum:
    case CollectionGrouping::GroupBy::OriginalYearAlbumDisc:
      return key == "0000" ? "Unknown" : key;
    case CollectionGrouping::GroupBy::Year:
    case CollectionGrouping::GroupBy::OriginalYear:
      return key == "0000" ? "Unknown" : std::to_string(std::atoi(key.c_str()));
    case CollectionGrouping::GroupBy::Samplerate:
    case CollectionGrouping::GroupBy::Bitdepth:
    case CollectionGrouping::GroupBy::Bitrate:
      return key == "000" ? "Unknown" : std::to_string(std::atoi(key.c_str()));
    case CollectionGrouping::GroupBy::None:
    case CollectionGrouping::GroupBy::GroupByCount:
      break;
  }
  return {};
}

inline Song FirstSong(const CollectionGrouping::Node &node) {
  if (!node.songs.empty()) {
    return node.songs.front();
  }
  for (const auto &child : node.children) {
    Song song = FirstSong(child);
    if (!song.url().empty()) {
      return song;
    }
  }
  return Song();
}

}  // namespace CollectionDivider

#endif
