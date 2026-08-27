#ifndef STRAWBERRY_PLAYLISTBEHAVIOUR_H
#define STRAWBERRY_PLAYLISTBEHAVIOUR_H

#include "core/song.h"
#include "playlist/playlistdelegates.h"

#include <algorithm>
#include <cstdlib>
#include <string>

namespace PlaylistBehaviour {

inline std::string NormalizeUrl(const std::string &url) {
  if (url.rfind("file://", 0) == 0 || url.find("://") != std::string::npos) {
    return url;
  }
  if (!url.empty() && url.front() == '/') {
    return "file://" + url;
  }
  return url;
}

inline bool UrlsMatch(const std::string &a, const std::string &b) { return NormalizeUrl(a) == NormalizeUrl(b); }

inline bool IsLocalUrl(const std::string &url) {
  if (url.empty()) {
    return false;
  }
  if (url.rfind("file://", 0) == 0) {
    return true;
  }
  return url.find("://") == std::string::npos;
}

inline bool IsLocalMedia(const Song &song) {
  if (song.is_stream() || song.is_cdda()) {
    return false;
  }
  if (song.source() == Song::Source::LocalFile || song.source() == Song::Source::Collection || song.source() == Song::Source::Unknown) {
    return IsLocalUrl(song.url());
  }
  return false;
}

inline bool ShouldGreyout(const Song &song) { return song.unavailable(); }

inline bool ApplyValidity(Song *song, bool valid) {
  if (!song) {
    return false;
  }
  const bool unavailable = !valid;
  if (song->unavailable() == unavailable) {
    return false;
  }
  song->set_unavailable(unavailable);
  return true;
}

inline bool ApplyLocalExistence(Song *song, bool exists) {
  if (!song || !IsLocalMedia(*song)) {
    return false;
  }
  return ApplyValidity(song, exists);
}

inline bool ShouldPromptClose(bool warn, bool favorite, bool empty) { return warn && !favorite && !empty; }

inline bool ShouldStopAfterError(bool continue_on_error, int errors, int playlist_rows) {
  if (!continue_on_error) {
    return true;
  }
  const int limit = std::max(1, playlist_rows);
  return errors >= std::min(limit, 100);
}

inline bool ColumnIsNumeric(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Track:
    case PlaylistColumn::Year:
    case PlaylistColumn::OriginalYear:
    case PlaylistColumn::Disc:
    case PlaylistColumn::Bitrate:
    case PlaylistColumn::Samplerate:
    case PlaylistColumn::Bitdepth:
    case PlaylistColumn::PlayCount:
    case PlaylistColumn::SkipCount:
    case PlaylistColumn::Length:
    case PlaylistColumn::Filesize:
    case PlaylistColumn::BPM:
    case PlaylistColumn::Rating:
    case PlaylistColumn::Queue:
      return true;
    default:
      return false;
  }
}

inline bool LessThanText(const std::string &left, const std::string &right, bool numeric, bool descending) {
  if (numeric) {
    const double ln = std::strtod(left.c_str(), nullptr);
    const double rn = std::strtod(right.c_str(), nullptr);
    return descending ? ln > rn : ln < rn;
  }
  return descending ? left > right : left < right;
}

}  // namespace PlaylistBehaviour

#endif
