#ifndef STRAWBERRY_SMARTPLAYLISTTERMROW_H
#define STRAWBERRY_SMARTPLAYLISTTERMROW_H

namespace SmartPlaylistTermRow {

// Qt SmartPlaylistSearchTermWidgetOverlay text.
inline const char *OverlayLabel() { return "Add search term"; }

// Qt SetActive(false) disables the term fields and shows the overlay instead.
inline bool RowSensitive(bool active) { return active; }

inline bool ShowsRemove(bool active) { return active; }

// Qt CreatePages adds one empty term after the placeholder. Edit restores one row per saved term.
inline int InitialActiveTerms(bool editing, int saved_terms) {
  if (editing) {
    return saved_terms < 0 ? 0 : saved_terms;
  }
  return 1;
}

inline bool KeepsPlaceholder() { return true; }

}  // namespace SmartPlaylistTermRow

#endif
