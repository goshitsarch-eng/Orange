#ifndef STRAWBERRY_RADIOBROWSERSEARCHOPTS_H
#define STRAWBERRY_RADIOBROWSERSEARCHOPTS_H

#include "constants/radiobrowsersettings.h"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace RadioBrowserSearchOpts {

struct SortOption {
  const char *label = "";
  const char *id = "";
};

inline std::vector<SortOption> SortOptions() {
  return {
      {"By votes", "votes"},
      {"By clicks", "clickcount"},
      {"By name", "name"},
      {"By bitrate", "bitrate"},
  };
}

inline int SortCount() { return static_cast<int>(SortOptions().size()); }

inline std::vector<std::pair<std::string, std::string>> SortChoices() {
  std::vector<std::pair<std::string, std::string>> choices;
  for (const SortOption &option : SortOptions()) {
    choices.emplace_back(option.id, option.label);
  }
  return choices;
}

inline const char *DefaultSort() { return RadioBrowserSettings::kDefaultSortDefault; }

inline int SortIndex(const char *id) {
  if (!id || !*id) {
    id = DefaultSort();
  }
  const std::vector<SortOption> options = SortOptions();
  for (int i = 0; i < static_cast<int>(options.size()); ++i) {
    if (std::strcmp(options[static_cast<size_t>(i)].id, id) == 0) {
      return i;
    }
  }
  return 0;
}

inline const char *SortId(int index) {
  const std::vector<SortOption> options = SortOptions();
  if (index < 0 || index >= static_cast<int>(options.size())) {
    return DefaultSort();
  }
  return options[static_cast<size_t>(index)].id;
}

inline bool ReverseOrder(const std::string &order) { return order != "name"; }

inline const char *AllCountriesLabel() { return "All countries"; }

inline const char *AllCountriesId() { return "all"; }

inline bool IsAllCountries(const char *id) { return !id || !*id || std::strcmp(id, AllCountriesId()) == 0; }

inline const char *SearchPlaceholder() { return "Search radio stations..."; }

inline const char *HelpText() { return "Search for radio stations using radio-browser.info"; }

inline const char *LoadMoreLabel() { return "Load more..."; }

inline const char *SearchingText() { return "Searching..."; }

inline const char *ChannelsTab() { return "Channels"; }

inline const char *BrowserTab() { return "Radio Browser"; }

inline int DebounceMs() { return 300; }

inline bool HasMore(int received, int limit) { return limit > 0 && received == limit; }

inline int NextOffset(int offset, int limit) { return offset + limit; }

inline std::string StatusText(int count, bool first_page_empty) {
  if (first_page_empty) {
    return "No stations found.";
  }
  return std::to_string(count) + " stations found";
}

inline std::string SearchFailed(const std::string &error) {
  if (error.empty()) {
    return "Radio Browser search failed.";
  }
  return "Radio Browser search failed: " + error;
}

}  // namespace RadioBrowserSearchOpts

#endif
