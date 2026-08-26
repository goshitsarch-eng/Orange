#include "scrobbler/scrobblemetadata.h"

ScrobbleMetadata ScrobbleMetadata::FromSong(const Song &song, uint64_t timestamp) {
  ScrobbleMetadata metadata;
  metadata.artist = song.artist();
  metadata.album = song.album();
  metadata.title = song.title();
  metadata.albumartist = song.EffectiveAlbumartist();
  metadata.track = song.track();
  metadata.length_nanosec = song.length_nanosec();
  metadata.timestamp = timestamp;
  return metadata;
}
