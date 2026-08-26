#ifndef STRAWBERRY_COLLECTIONFILTEROPTIONS_H
#define STRAWBERRY_COLLECTIONFILTEROPTIONS_H

#include "core/song.h"

#include <string>

class CollectionFilterOptions {
 public:
  enum class FilterMode {
    All,
    Duplicates,
    Untagged
  };

  CollectionFilterOptions();

  FilterMode filter_mode() const { return filter_mode_; }
  int max_age() const { return max_age_; }
  float min_rating() const { return min_rating_; }
  const std::string &filter_text() const { return filter_text_; }
  bool has_filter_text() const { return has_filter_text_; }

  void set_filter_mode(FilterMode filter_mode);
  void set_max_age(int max_age) { max_age_ = max_age; }
  void set_min_rating(float min_rating) { min_rating_ = min_rating; }
  void set_filter_text(const std::string &filter_text);

  bool Matches(const Song &song) const;

 private:
  FilterMode filter_mode_ = FilterMode::All;
  int max_age_ = -1;
  float min_rating_ = -1.0f;
  std::string filter_text_;
  bool has_filter_text_ = false;
};

#endif
