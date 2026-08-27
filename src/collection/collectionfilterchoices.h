#ifndef STRAWBERRY_COLLECTIONFILTERCHOICES_H
#define STRAWBERRY_COLLECTIONFILTERCHOICES_H

#include "collection/collectionfilteroptions.h"

namespace CollectionFilterChoices {

inline constexpr const char *kAgeLabels[] = {"Entire collection",        "Added today",
                                             "Added this week",         "Added this month",
                                             "Added within three months", "Added this year"};
inline constexpr int kAgeSeconds[] = {-1, 60 * 60 * 24, 60 * 60 * 24 * 7, 60 * 60 * 24 * 30, 60 * 60 * 24 * 30 * 3, 60 * 60 * 24 * 365};
inline constexpr int kAgeCount = static_cast<int>(sizeof(kAgeLabels) / sizeof(kAgeLabels[0]));

inline constexpr const char *kRatingLabels[] = {"Any rating",
                                                "Rating non null",
                                                "Rating greater than 1 star",
                                                "Rating greater than 2 stars",
                                                "Rating greater than 3 stars",
                                                "Rating greater than 4 stars"};
inline constexpr float kRatingValues[] = {-1.0f, 0.0f, 0.2f, 0.4f, 0.6f, 0.8f};
inline constexpr int kRatingCount = static_cast<int>(sizeof(kRatingLabels) / sizeof(kRatingLabels[0]));

inline constexpr const char *kModeLabels[] = {"Show all songs", "Show only duplicates", "Show only untagged"};
inline constexpr int kModeCount = static_cast<int>(sizeof(kModeLabels) / sizeof(kModeLabels[0]));

inline const char *AgeMenuTitle() { return "Filter by age"; }
inline const char *RatingMenuTitle() { return "Filter by rating"; }

inline int ClampIndex(int index, int count) {
  if (index < 0) {
    return 0;
  }
  if (index >= count) {
    return count - 1;
  }
  return index;
}

inline CollectionFilterOptions FromIndices(int age, int rating, int mode) {
  CollectionFilterOptions options;
  const int age_index = ClampIndex(age, kAgeCount);
  const int rating_index = ClampIndex(rating, kRatingCount);
  const int mode_index = ClampIndex(mode, kModeCount);
  options.set_max_age(kAgeSeconds[age_index]);
  options.set_min_rating(kRatingValues[rating_index]);
  if (mode_index == 1) {
    options.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  } else if (mode_index == 2) {
    options.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  } else {
    options.set_filter_mode(CollectionFilterOptions::FilterMode::All);
  }
  return options;
}

}  // namespace CollectionFilterChoices

#endif  // STRAWBERRY_COLLECTIONFILTERCHOICES_H
