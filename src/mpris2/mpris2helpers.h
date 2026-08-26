#ifndef STRAWBERRY_MPRIS2HELPERS_H
#define STRAWBERRY_MPRIS2HELPERS_H

#include "core/song.h"
#include "playlist/playlistsequence.h"

#include <cstdlib>
#include <string>

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
