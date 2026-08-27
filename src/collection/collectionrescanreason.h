#ifndef STRAWBERRY_COLLECTIONRESCANREASON_H
#define STRAWBERRY_COLLECTIONRESCANREASON_H

#include "core/song.h"

namespace CollectionRescanReason {

inline bool MissingFingerprint(const Song &song, bool song_tracking) {
  return song_tracking && song.fingerprint().empty();
}

inline bool MissingLoudness(const Song &song, bool ebu_analysis) {
  return ebu_analysis && (!song.ebur128_integrated_loudness_lufs() || !song.ebur128_loudness_range_lu());
}

inline bool NeedsAnalysisRescan(const Song &song, bool song_tracking, bool ebu_analysis) {
  return MissingFingerprint(song, song_tracking) || MissingLoudness(song, ebu_analysis);
}

}  // namespace CollectionRescanReason

#endif  // STRAWBERRY_COLLECTIONRESCANREASON_H
