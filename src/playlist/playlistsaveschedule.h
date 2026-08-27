#ifndef STRAWBERRY_PLAYLISTSAVESCHEDULE_H
#define STRAWBERRY_PLAYLISTSAVESCHEDULE_H

namespace PlaylistSaveSchedule {

constexpr unsigned kDelayMs = 900;

enum class Intent { None, LastPlayed, Items, Full };

inline bool ShouldSchedule(bool loading, bool has_id) { return !loading && has_id; }

inline Intent Merge(Intent pending, Intent next) {
  if (pending == Intent::Full || next == Intent::Full) {
    return Intent::Full;
  }
  if (pending == Intent::Items || next == Intent::Items) {
    return Intent::Items;
  }
  if (pending == Intent::LastPlayed || next == Intent::LastPlayed) {
    return Intent::LastPlayed;
  }
  return Intent::None;
}

inline bool IsImmediate(Intent intent, bool has_id) { return !has_id || intent == Intent::None; }

}  // namespace PlaylistSaveSchedule

#endif  // STRAWBERRY_PLAYLISTSAVESCHEDULE_H
