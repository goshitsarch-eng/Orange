#ifndef STRAWBERRY_SCROBBLERLOVESTATE_H
#define STRAWBERRY_SCROBBLERLOVESTATE_H

#include "core/song.h"

namespace ScrobblerLoveState {

inline bool CanLove(bool scrobbler_enabled, const Song &song) { return scrobbler_enabled && song.is_metadata_good(); }

inline bool DisableAfterLove() { return true; }

inline bool ResetLovedOnSongChange(const std::string &previous_url, const std::string &next_url) { return previous_url != next_url; }

}  // namespace ScrobblerLoveState

#endif
