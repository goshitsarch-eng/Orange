#ifndef STRAWBERRY_STREAMINGMETADATAMERGE_H
#define STRAWBERRY_STREAMINGMETADATAMERGE_H

#include "core/song.h"

namespace StreamingMetadataMerge {

// Qt MainWindow::ProcessMetadataQueue: only overwrite fields the dedicated request actually filled.
inline bool ShouldApply(const Song &fetched) { return fetched.is_valid(); }

inline void Apply(Song *target, const Song &fetched) {
  if (!target || !ShouldApply(fetched)) {
    return;
  }
  if (!fetched.title().empty()) {
    target->set_title(fetched.title());
  }
  if (!fetched.artist().empty()) {
    target->set_artist(fetched.artist());
  }
  if (!fetched.album().empty()) {
    target->set_album(fetched.album());
  }
  if (!fetched.albumartist().empty()) {
    target->set_albumartist(fetched.albumartist());
  }
  if (!fetched.genre().empty()) {
    target->set_genre(fetched.genre());
  }
  if (!fetched.composer().empty()) {
    target->set_composer(fetched.composer());
  }
  if (!fetched.performer().empty()) {
    target->set_performer(fetched.performer());
  }
  if (!fetched.comment().empty()) {
    target->set_comment(fetched.comment());
  }
  if (fetched.track() > 0) {
    target->set_track(fetched.track());
  }
  if (fetched.disc() > 0) {
    target->set_disc(fetched.disc());
  }
  if (fetched.year() > 0) {
    target->set_year(fetched.year());
  }
  if (fetched.length_nanosec() > 0) {
    target->set_length_nanosec(fetched.length_nanosec());
  }
  if (!fetched.art_automatic().empty()) {
    target->set_art_automatic(fetched.art_automatic());
  }
}

}  // namespace StreamingMetadataMerge

#endif
