#ifndef STRAWBERRY_TRACKSELECTIONLABELS_H
#define STRAWBERRY_TRACKSELECTIONLABELS_H

namespace TrackSelectionLabels {

inline const char *Title() { return "Tag fetcher"; }
inline const char *SelectBest() { return "Select best possible match"; }
inline const char *NoResults() { return "No results"; }
inline const char *UnableToFind() { return "Strawberry was unable to find results for this file"; }
inline const char *Error() { return "Error"; }
inline const char *OriginalTags() { return "Original tags"; }

inline const char *SavingTracks() { return "Saving tracks..."; }

inline bool ShowEmptyResults(bool pending, bool has_results) { return !pending && !has_results; }

// Qt TrackSelectionDialog::SetLoading — lock chrome while tags are written.
inline bool ButtonsEnabled(bool loading) { return !loading; }

inline bool SplitterEnabled(bool loading) { return !loading; }

inline bool LoadingVisible(bool loading) { return loading; }

// Qt TrackSelectionDialog::Init — song list and Prev/Next only for multi-song.
inline bool SongListVisible(int song_count) { return song_count > 1; }

inline bool NavEnabled(int song_count) { return song_count > 1; }

inline bool ApplyAllVisible(int song_count) { return song_count > 1; }

}  // namespace TrackSelectionLabels

#endif
