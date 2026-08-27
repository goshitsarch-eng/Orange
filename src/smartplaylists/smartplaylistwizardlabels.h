#ifndef STRAWBERRY_SMARTPLAYLISTWIZARDLABELS_H
#define STRAWBERRY_SMARTPLAYLISTWIZARDLABELS_H

#include "smartplaylists/smartplaylist.h"

#include <string>
#include <utility>
#include <vector>

namespace SmartPlaylistWizardLabels {

inline const char *SearchMode() { return "Search mode"; }
inline const char *And() { return "Match every search term (AND)"; }
inline const char *Or() { return "Match one or more search terms (OR)"; }
inline const char *All() { return "Include all songs"; }
inline const char *SearchTerms() { return "Search terms"; }
inline const char *Sorting() { return "Sorting"; }
inline const char *Random() { return "Put songs in a random order"; }
inline const char *SortBy() { return "Sort songs by"; }
inline const char *Limits() { return "Limits"; }
inline const char *ShowAll() { return "Show all the songs"; }
inline const char *OnlyFirst() { return "Only show the first"; }
inline const char *Songs() { return " songs"; }
inline const char *Name() { return "Name"; }
inline const char *UseDynamic() { return "Use dynamic mode"; }
inline const char *DynamicHint() {
  return "In dynamic mode new tracks will be chosen and added to the playlist every time a song finishes.";
}

inline std::vector<std::pair<std::string, std::string>> SearchTypeChoices() {
  return {{"and", And()}, {"or", Or()}, {"all", All()}};
}

inline int TypeIndex(SmartPlaylistSearch::SearchType type) {
  switch (type) {
    case SmartPlaylistSearch::SearchType::Or:
      return 1;
    case SmartPlaylistSearch::SearchType::All:
      return 2;
    case SmartPlaylistSearch::SearchType::And:
    default:
      return 0;
  }
}

inline SmartPlaylistSearch::SearchType TypeFromIndex(int index) {
  if (index == 1) {
    return SmartPlaylistSearch::SearchType::Or;
  }
  if (index == 2) {
    return SmartPlaylistSearch::SearchType::All;
  }
  return SmartPlaylistSearch::SearchType::And;
}

inline bool TermsSensitive(SmartPlaylistSearch::SearchType type) { return SmartPlaylistSearch::TermsApply(type); }

inline bool ShowAllSongs(int limit) { return limit <= 0; }

inline int LimitFromUi(bool show_all, int first) { return show_all || first <= 0 ? 0 : first; }

inline int LimitSpinOrDefault(int limit, int fallback = 15) { return limit > 0 ? limit : fallback; }

inline bool FieldSortSensitive(bool random) { return !random; }

inline bool LimitSpinSensitive(bool show_all) { return !show_all; }

}  // namespace SmartPlaylistWizardLabels

#endif
