#ifndef STRAWBERRY_PLAYERITEMOPTIONS_H
#define STRAWBERRY_PLAYERITEMOPTIONS_H

#include "core/song.h"
#include "playlist/playlistitem.h"

namespace PlayerItemOptions {

inline int Bits(PlaylistItem::Option options) { return static_cast<int>(options); }

inline bool Has(PlaylistItem::Option options, PlaylistItem::Option flag) { return (Bits(options) & Bits(flag)) != 0; }

inline PlaylistItem::Option Combine(PlaylistItem::Option left, PlaylistItem::Option right) {
  return static_cast<PlaylistItem::Option>(Bits(left) | Bits(right));
}

inline bool PauseDisabled(Song::Source source) { return source == Song::Source::Stream || source == Song::Source::SomaFM ||
                                                       source == Song::Source::RadioParadise || source == Song::Source::RadioBrowser; }

inline bool SeekDisabled(const Song &song) { return song.is_radio(); }

inline bool PauseDisabled(const Song &song) { return PauseDisabled(song.source()) || song.is_radio(); }

inline PlaylistItem::Option ForSong(const Song &song) {
  PlaylistItem::Option options = PlaylistItem::Option::Default;
  if (SeekDisabled(song)) {
    options = Combine(options, PlaylistItem::Option::SeekDisabled);
  }
  if (PauseDisabled(song)) {
    options = Combine(options, PlaylistItem::Option::PauseDisabled);
  }
  return options;
}

inline bool ShouldStopInsteadOfPause(const Song &song) { return PauseDisabled(song); }

inline bool ShouldIgnoreSeek(const Song &song) { return SeekDisabled(song); }

}  // namespace PlayerItemOptions

#endif  // STRAWBERRY_PLAYERITEMOPTIONS_H
