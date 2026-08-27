#ifndef STRAWBERRY_TRACKSLIDERSTATE_H
#define STRAWBERRY_TRACKSLIDERSTATE_H

#include "core/playeritemoptions.h"
#include "core/song.h"
#include "playlist/playlistitem.h"

namespace TrackSliderState {

inline const char *StoppedLabel() { return "0:00:00"; }

inline bool SliderEnabled(bool stopped, bool can_seek) { return !stopped && can_seek; }

inline bool LabelsEnabled(bool stopped) { return !stopped; }

inline bool CanSeekFromOptions(PlaylistItem::Option options) {
  return !PlayerItemOptions::Has(options, PlaylistItem::Option::SeekDisabled);
}

inline bool CanSeekFromSong(const Song &song) { return !PlayerItemOptions::ShouldIgnoreSeek(song); }

inline bool ShouldAcceptSeek(bool stopped, bool can_seek) { return SliderEnabled(stopped, can_seek); }

inline bool ShouldUpdateTimes(bool stopped) { return !stopped; }

}  // namespace TrackSliderState

#endif  // STRAWBERRY_TRACKSLIDERSTATE_H
