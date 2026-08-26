#include "streaming/streamingsearchmodel.h"

#include "utilities/strutils.h"

#include <algorithm>

void StreamingSearchModel::SetSongs(const SongList &songs) { songs_ = songs; }

void StreamingSearchModel::SetFilter(const std::string &filter) { filter_ = filter; }

void StreamingSearchModel::SetSort(SortField field, bool descending) {
  sort_field_ = field;
  descending_ = descending;
}

void StreamingSearchModel::SetSearchType(StreamingService::SearchType type) { search_type_ = type; }

int StreamingSearchModel::Compare(const Song &left, const Song &right, SortField field) {
  auto key = [field](const Song &song) {
    switch (field) {
      case SortField::Artist:
        return StrUtils::ToLower(song.EffectiveAlbumartist().empty() ? song.artist() : song.EffectiveAlbumartist());
      case SortField::Album:
        return StrUtils::ToLower(song.album());
      case SortField::Title:
        break;
    }
    return StrUtils::ToLower(song.title());
  };
  const std::string a = key(left);
  const std::string b = key(right);
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}

SongList StreamingSearchModel::Visible() const {
  SongList visible = songs_;
  if (!filter_.empty()) {
    SongList filtered;
    for (const Song &song : visible) {
      if (StrUtils::ContainsInsensitive(song.PrettyTitleWithArtist(), filter_) || StrUtils::ContainsInsensitive(song.album(), filter_)) {
        filtered.push_back(song);
      }
    }
    visible = std::move(filtered);
  }
  std::sort(visible.begin(), visible.end(), [this](const Song &left, const Song &right) {
    const int order = Compare(left, right, sort_field_);
    return descending_ ? order > 0 : order < 0;
  });
  return visible;
}
