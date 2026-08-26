#include "collection/collectionfilteroptions.h"

#include "utilities/strutils.h"

#include <cstdint>
#include <ctime>

CollectionFilterOptions::CollectionFilterOptions() = default;

void CollectionFilterOptions::set_filter_mode(FilterMode filter_mode) {
  filter_mode_ = filter_mode;
  filter_text_.clear();
  has_filter_text_ = false;
}

void CollectionFilterOptions::set_filter_text(const std::string &filter_text) {
  filter_mode_ = FilterMode::All;
  filter_text_ = filter_text;
  has_filter_text_ = true;
}

bool CollectionFilterOptions::Matches(const Song &song) const {
  if (max_age_ != -1) {
    const int64_t cutoff = static_cast<int64_t>(std::time(nullptr)) - max_age_;
    if (song.ctime() <= cutoff) {
      return false;
    }
  }
  if (min_rating_ >= 0.0f) {
    if (song.rating() <= min_rating_) {
      return false;
    }
  }
  if (has_filter_text_) {
    return StrUtils::ContainsInsensitive(song.albumartist(), filter_text_) || StrUtils::ContainsInsensitive(song.artist(), filter_text_) ||
           StrUtils::ContainsInsensitive(song.album(), filter_text_) || StrUtils::ContainsInsensitive(song.title(), filter_text_);
  }
  return true;
}
