#ifndef STRAWBERRY_TRACKSELECTIONLABELS_H
#define STRAWBERRY_TRACKSELECTIONLABELS_H

namespace TrackSelectionLabels {

inline const char *Title() { return "Tag fetcher"; }
inline const char *SelectBest() { return "Select best possible match"; }
inline const char *NoResults() { return "No results"; }
inline const char *UnableToFind() { return "Strawberry was unable to find results for this file"; }
inline const char *Error() { return "Error"; }
inline const char *OriginalTags() { return "Original tags"; }

inline bool ShowEmptyResults(bool pending, bool has_results) { return !pending && !has_results; }

}  // namespace TrackSelectionLabels

#endif
