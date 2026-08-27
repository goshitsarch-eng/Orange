#ifndef STRAWBERRY_SMARTPLAYLISTPREVIEWPOLICY_H
#define STRAWBERRY_SMARTPLAYLISTPREVIEWPOLICY_H

#include "smartplaylists/smartplaylist.h"

namespace SmartPlaylistPreviewPolicy {

enum class Kind { Terms, Sort };

// Qt SmartPlaylistQueryWizardPlugin::UpdateTermPreview skips when the search is invalid but term widgets remain.
// Removing the last term still refreshes. Sort preview skips any invalid search.
inline bool ShouldUpdate(Kind kind, bool search_valid, bool has_term_widgets) {
  if (kind == Kind::Sort) {
    return search_valid;
  }
  return search_valid || !has_term_widgets;
}

// Qt term-page preview clears the limit (limit_ = -1). GTK uses <= 0 for show-all.
inline int LimitForPreview(Kind kind, int limit) { return kind == Kind::Terms ? 0 : limit; }

inline bool SameTerm(const SmartPlaylistTerm &a, const SmartPlaylistTerm &b) {
  return a.field == b.field && a.op == b.op && a.value == b.value && a.second_value == b.second_value && a.date_type == b.date_type;
}

inline bool SameSearch(const SmartPlaylistSearch &a, const SmartPlaylistSearch &b) {
  if (a.type != b.type || a.limit != b.limit || a.sort_field != b.sort_field || a.sort_descending != b.sort_descending ||
      a.sort_random != b.sort_random || a.terms.size() != b.terms.size()) {
    return false;
  }
  for (size_t i = 0; i < a.terms.size(); ++i) {
    if (!SameTerm(a.terms[i], b.terms[i])) {
      return false;
    }
  }
  return true;
}

inline SmartPlaylistSearch SearchForPreview(SmartPlaylistSearch search, Kind kind) {
  search.limit = LimitForPreview(kind, search.limit);
  return search;
}

}  // namespace SmartPlaylistPreviewPolicy

#endif
