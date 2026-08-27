#ifndef STRAWBERRY_LYRICSFETCHERPACING_H
#define STRAWBERRY_LYRICSFETCHERPACING_H

#include "core/song.h"
#include "lyrics/lyricssearchrequest.h"

namespace LyricsFetcherPacing {

constexpr int kMaxConcurrent = 5;
constexpr int kStarterDelayMs = 500;

inline bool CanStartMore(int active, int max_concurrent = kMaxConcurrent) { return active < max_concurrent; }

inline LyricsSearchRequest Normalize(LyricsSearchRequest request) {
  request.album = Song::AlbumRemoveDiscMisc(request.album);
  request.title = Song::TitleRemoveMisc(request.title);
  return request;
}

}  // namespace LyricsFetcherPacing

#endif  // STRAWBERRY_LYRICSFETCHERPACING_H
