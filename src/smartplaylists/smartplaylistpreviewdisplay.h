#ifndef STRAWBERRY_SMARTPLAYLISTPREVIEWDISPLAY_H
#define STRAWBERRY_SMARTPLAYLISTPREVIEWDISPLAY_H

#include "core/song.h"
#include "playlist/playlistdelegates.h"
#include "smartplaylists/playlistgenerator.h"

#include <algorithm>
#include <string>
#include <vector>

namespace SmartPlaylistPreviewDisplay {

// Qt SmartPlaylistSearchPreview::SearchFinished displays at most PlaylistGenerator::kDefaultLimit rows.
inline int DisplayLimit() { return PlaylistGenerator::kDefaultLimit; }

inline int ShownCount(int total) { return std::min(total, DisplayLimit()); }

inline SongList SliceForDisplay(const SongList &songs) {
  if (static_cast<int>(songs.size()) <= DisplayLimit()) {
    return songs;
  }
  return SongList(songs.begin(), songs.begin() + DisplayLimit());
}

// Qt count_label: "%1 songs found" or "%1 songs found (showing %2)" when the display cap truncates.
inline const char *CountTemplate(bool truncated) { return truncated ? "%1 songs found (showing %2)" : "%1 songs found"; }

inline std::string ApplyCountTemplate(const std::string &text, int total, int shown) {
  std::string out = text;
  const std::string total_text = std::to_string(total);
  const std::string shown_text = std::to_string(shown);
  size_t pos = out.find("%1");
  if (pos != std::string::npos) {
    out.replace(pos, 2, total_text);
  }
  pos = out.find("%2");
  if (pos != std::string::npos) {
    out.replace(pos, 2, shown_text);
  }
  return out;
}

inline std::string CountLabel(int total, int shown) { return ApplyCountTemplate(CountTemplate(shown < total), total, shown); }

inline std::string CountLabelForTotal(int total) { return CountLabel(total, ShownCount(total)); }

// Compact read-only columns. Qt uses the user's PlaylistView header; these are the identity fields always shown.
inline std::vector<PlaylistColumn> Columns() {
  return {PlaylistColumn::Title, PlaylistColumn::Artist, PlaylistColumn::Album, PlaylistColumn::Length};
}

inline std::string CellText(const Song &song, PlaylistColumn column) { return PlaylistDelegates::ColumnText(song, column); }

inline std::string RowText(const Song &song) {
  std::string text;
  for (PlaylistColumn column : Columns()) {
    if (!text.empty()) {
      text += "  ";
    }
    text += CellText(song, column);
  }
  return text;
}

// Qt Update stores pending_search_ when generator_ is busy or the widget is hidden.
inline bool ShouldDefer(bool busy, bool hidden) { return busy || hidden; }

// Qt showEvent runs the pending search once the widget is shown and idle.
inline bool ShouldRunPendingOnShow(bool pending_valid, bool busy) { return pending_valid && !busy; }

// Qt SearchFinished discards results when a different search arrived while Generate() ran.
inline bool ShouldDiscardForPending(bool pending_valid, bool pending_same_as_finished) { return pending_valid && !pending_same_as_finished; }

// Qt busy_container label.
inline const char *BusyText() { return "Loading..."; }

enum class UpdateAction { Ignore, Defer, Run };

// Qt Update: same as last completed search is ignored; busy/hidden stores pending.
inline UpdateAction DecideUpdate(bool same_as_last, bool busy, bool hidden) {
  if (same_as_last) {
    return UpdateAction::Ignore;
  }
  if (ShouldDefer(busy, hidden)) {
    return UpdateAction::Defer;
  }
  return UpdateAction::Run;
}

}  // namespace SmartPlaylistPreviewDisplay

#endif
