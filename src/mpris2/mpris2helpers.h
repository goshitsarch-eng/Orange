#ifndef STRAWBERRY_MPRIS2HELPERS_H
#define STRAWBERRY_MPRIS2HELPERS_H

#include "core/song.h"
#include "playlist/playlistsequence.h"

#include <cstdint>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

namespace Mpris2Helpers {

inline std::string TrackId(const Song &song) {
  if (song.id() > 0) {
    return "/org/strawberrymusicplayer/Strawberry/Track/" + std::to_string(song.id());
  }
  return "/org/strawberrymusicplayer/Strawberry/Track/0";
}

inline std::string TrackIdForRow(const Song &song, int row) {
  if (song.id() > 0) {
    return TrackId(song);
  }
  return "/org/strawberrymusicplayer/Strawberry/Track/row" + std::to_string(row);
}

inline int RowFromTrackId(const std::string &track_id) {
  const std::string prefix = "/org/strawberrymusicplayer/Strawberry/Track/row";
  if (track_id.rfind(prefix, 0) != 0) {
    return -1;
  }
  return std::atoi(track_id.c_str() + prefix.size());
}

inline std::string ArtUrl(const Song &song) {
  if (!song.art_manual().empty()) {
    return song.art_manual();
  }
  return song.art_automatic();
}

inline std::string LoopStatus(PlaylistSequence::RepeatMode mode) {
  switch (mode) {
    case PlaylistSequence::RepeatMode::Track:
      return "Track";
    case PlaylistSequence::RepeatMode::Playlist:
    case PlaylistSequence::RepeatMode::Album:
      return "Playlist";
    default:
      return "None";
  }
}

inline int64_t PositionUsec(int64_t position_nanosec) { return position_nanosec / 1000; }

inline const char *kNoTrack = "/org/mpris/MediaPlayer2/TrackList/NoTrack";

struct TrackListDiff {
  enum class Kind { None, Incremental, Replaced };
  Kind kind = Kind::None;
  std::vector<std::string> added;
  std::vector<std::string> after_track;
  std::vector<std::string> removed;
};

inline TrackListDiff DiffTrackIds(const std::vector<std::string> &before, const std::vector<std::string> &after) {
  TrackListDiff diff;
  if (before == after) {
    return diff;
  }
  std::set<std::string> before_set(before.begin(), before.end());
  std::set<std::string> after_set(after.begin(), after.end());
  for (const auto &id : before) {
    if (!after_set.count(id)) {
      diff.removed.push_back(id);
    }
  }
  for (size_t i = 0; i < after.size(); ++i) {
    if (!before_set.count(after[i])) {
      diff.added.push_back(after[i]);
      diff.after_track.push_back(i == 0 ? kNoTrack : after[i - 1]);
    }
  }
  std::vector<std::string> before_kept;
  std::vector<std::string> after_kept;
  for (const auto &id : before) {
    if (after_set.count(id)) {
      before_kept.push_back(id);
    }
  }
  for (const auto &id : after) {
    if (before_set.count(id)) {
      after_kept.push_back(id);
    }
  }
  if (before_kept != after_kept || diff.added.size() + diff.removed.size() > 16) {
    diff.kind = TrackListDiff::Kind::Replaced;
    return diff;
  }
  diff.kind = TrackListDiff::Kind::Incremental;
  return diff;
}

inline bool MetadataNeedsUpdate(const Song &before, const Song &after) {
  return before.title() != after.title() || before.artist() != after.artist() || before.album() != after.album() ||
         before.url() != after.url() || before.length_nanosec() != after.length_nanosec() || ArtUrl(before) != ArtUrl(after);
}

inline PlaylistSequence::RepeatMode RepeatFromLoopStatus(const std::string &status) {
  if (status == "Track") {
    return PlaylistSequence::RepeatMode::Track;
  }
  if (status == "Playlist") {
    return PlaylistSequence::RepeatMode::Playlist;
  }
  return PlaylistSequence::RepeatMode::Off;
}

}  // namespace Mpris2Helpers

#endif
