#ifndef STRAWBERRY_MPRIS2HELPERS_H
#define STRAWBERRY_MPRIS2HELPERS_H

#include "core/song.h"
#include "engine/enginebase.h"
#include "playlist/playlist.h"
#include "playlist/playlistsequence.h"

#include <cstdint>
#include <cstdlib>
#include <ctime>
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

inline std::string ArtUrlOrOverride(const Song &song, const std::string &override_url) {
  const std::string url = ArtUrl(song);
  return url.empty() ? override_url : url;
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

// Qt Song::ToXesam: title is PrettyTitle, url is effective_url (stream URL when set).
inline std::string XesamTitle(const Song &song) { return song.PrettyTitle(); }

inline std::string XesamUrl(const Song &song) { return song.stream_url(); }

inline bool MetadataNeedsUpdate(const Song &before, const Song &after) {
  return XesamTitle(before) != XesamTitle(after) || before.artist() != after.artist() || before.album() != after.album() ||
         XesamUrl(before) != XesamUrl(after) || before.length_nanosec() != after.length_nanosec() || ArtUrl(before) != ArtUrl(after) ||
         before.rating() != after.rating() || before.bitrate() != after.bitrate() || before.genre() != after.genre() ||
         before.disc() != after.disc() || before.comment() != after.comment() || before.composer() != after.composer() ||
         before.playcount() != after.playcount() || before.ctime() != after.ctime() || before.lastplayed() != after.lastplayed() ||
         before.year() != after.year() || before.track() != after.track() || before.albumartist() != after.albumartist();
}

inline bool CanPlay(const Playlist *playlist) { return playlist && playlist->row_count() > 0; }

inline bool CanPause(EngineBase::State state) {
  return state == EngineBase::State::Playing || state == EngineBase::State::Paused || state == EngineBase::State::Idle;
}

inline bool CanGoNext(const Playlist *playlist) { return playlist && playlist->PeekNextRow() != -1; }

inline bool PreviousWouldRestartTrack(int64_t position_nanosec) { return position_nanosec > 3 * 1000000000LL; }

inline bool CanGoPrevious(const Playlist *playlist, int64_t position_nanosec = 0) {
  if (!playlist || playlist->row_count() == 0) {
    return false;
  }
  return playlist->PeekPreviousRow() != playlist->current_row() || PreviousWouldRestartTrack(position_nanosec);
}

inline bool CanSeek(const Song &song, EngineBase::State state) {
  return song.is_valid() && state != EngineBase::State::Empty && !song.is_stream();
}

inline bool SetPositionAllowed(const std::string &requested_id, const std::string &current_id, int64_t position_us, int64_t length_nanosec,
                               bool can_seek) {
  return can_seek && !requested_id.empty() && requested_id == current_id && position_us >= 0 &&
         (length_nanosec <= 0 || position_us * 1000 < length_nanosec);
}

// Qt Mpris2::Rating: unset/negative ratings are exposed as 0.
inline double RatingProperty(float rating) { return rating <= 0 ? 0.0 : static_cast<double>(rating); }

// Qt Mpris2::SetRating: clamp above 1.0 and treat 0 or below as unset (-1).
inline float RatingFromProperty(double rating) {
  if (rating > 1.0) {
    return 1.0f;
  }
  if (rating <= 0.0) {
    return -1.0f;
  }
  return static_cast<float>(rating);
}

inline bool ShouldAddUserRating(float rating) { return rating != -1.0f; }

inline bool ShouldAddBitrate(int bitrate) { return bitrate > 0; }

inline bool ShouldAddString(const std::string &value) { return !value.empty(); }

inline bool ShouldAddPositiveInt(int value) { return value > 0; }

inline bool ShouldAddPlaycount(unsigned playcount) { return playcount > 0; }

inline bool ShouldAddYear(int year) { return year > 0; }

// Qt mpris::AsMPRISDateTimeType: omit -1, otherwise ISO-8601 UTC.
inline std::string AsMprisDateTime(int64_t unix_secs) {
  if (unix_secs == -1) {
    return {};
  }
  const std::time_t t = static_cast<std::time_t>(unix_secs);
  std::tm tm{};
  if (!gmtime_r(&t, &tm)) {
    return {};
  }
  char buf[32] = {};
  if (std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm) == 0) {
    return {};
  }
  return buf;
}

inline std::vector<std::string> ExtraXesamKeys(const Song &song) {
  std::vector<std::string> keys;
  if (ShouldAddString(song.genre())) {
    keys.emplace_back("xesam:genre");
  }
  if (ShouldAddPositiveInt(song.disc())) {
    keys.emplace_back("xesam:discNumber");
  }
  if (ShouldAddString(song.comment())) {
    keys.emplace_back("xesam:comment");
  }
  if (!AsMprisDateTime(song.ctime()).empty()) {
    keys.emplace_back("xesam:contentCreated");
  }
  if (!AsMprisDateTime(song.lastplayed()).empty()) {
    keys.emplace_back("xesam:lastUsed");
  }
  if (ShouldAddString(song.composer())) {
    keys.emplace_back("xesam:composer");
  }
  if (ShouldAddPlaycount(song.playcount())) {
    keys.emplace_back("xesam:useCount");
  }
  if (ShouldAddYear(song.year())) {
    keys.emplace_back("year");
  }
  return keys;
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
