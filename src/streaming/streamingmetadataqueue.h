#ifndef STRAWBERRY_STREAMINGMETADATAQUEUE_H
#define STRAWBERRY_STREAMINGMETADATAQUEUE_H

#include "core/song.h"
#include "streaming/streamingmediaid.h"

#include <algorithm>
#include <string>
#include <vector>

namespace StreamingMetadataQueue {

// Qt MainWindow metadata_queue_timer_ interval.
inline constexpr int kDelayMs = 200;

struct Entry {
  Song::Source source = Song::Source::Unknown;
  std::string track_id;
  int row = -1;
  std::string url;
};

inline std::string UrlAfterScheme(const std::string &url) {
  const auto scheme = url.find("://");
  if (scheme != std::string::npos) {
    std::string rest = url.substr(scheme + 3);
    while (!rest.empty() && rest.front() == '/') {
      rest.erase(rest.begin());
    }
    return rest;
  }
  const auto colon = url.find(':');
  if (colon != std::string::npos) {
    return url.substr(colon + 1);
  }
  return url;
}

// Qt: song_id, else QUrl path (qobuz://7 stores the id in the host; path is empty).
inline std::string QobuzTrackId(const std::string &song_id, const std::string &url) {
  if (!song_id.empty()) {
    return song_id;
  }
  const std::string path = UrlAfterScheme(url);
  if (path.empty()) {
    return {};
  }
  const auto slash = path.rfind('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

// Qt: song_id, else scheme == spotify && path.startsWith("track:").
inline std::string SpotifyTrackId(const std::string &song_id, const std::string &url) {
  if (!song_id.empty()) {
    return song_id;
  }
  const std::string path = UrlAfterScheme(url);
  if (path.size() > 6 && path.compare(0, 6, "track:") == 0) {
    return path.substr(6);
  }
  if (url.rfind("spotify:", 0) == 0) {
    const std::string id = StreamingMediaId(url);
    if (id.rfind("track:", 0) == 0) {
      return id.substr(6);
    }
    if (!id.empty() && id.find(':') == std::string::npos) {
      return id;
    }
  }
  return {};
}

inline std::string TrackId(Song::Source source, const std::string &song_id, const std::string &url) {
  if (source == Song::Source::Qobuz) {
    return QobuzTrackId(song_id, url);
  }
  if (source == Song::Source::Spotify) {
    return SpotifyTrackId(song_id, url);
  }
  return {};
}

inline std::string TrackId(const Song &song) { return TrackId(song.source(), song.song_id(), song.url()); }

// Qt only queues Qobuz and Spotify dedicated metadata requests.
inline bool HandlesSource(Song::Source source) { return source == Song::Source::Qobuz || source == Song::Source::Spotify; }

inline bool ShouldEnqueue(Song::Source source, const std::string &track_id) { return HandlesSource(source) && !track_id.empty(); }

inline bool ShouldEnqueue(const Song &song) { return ShouldEnqueue(song.source(), TrackId(song)); }

inline bool ShouldStart(bool queue_empty, bool timer_active) { return !queue_empty && !timer_active; }

inline bool ShouldContinue(bool queue_empty) { return !queue_empty; }

inline Entry MakeEntry(const Song &song, int row) {
  Entry entry;
  entry.source = song.source();
  entry.track_id = TrackId(song);
  entry.row = row;
  entry.url = song.url();
  return entry;
}

inline std::vector<Entry> EntriesFromSelection(const SongList &songs, const std::vector<int> &rows) {
  std::vector<Entry> out;
  const size_t n = std::min(songs.size(), rows.size());
  for (size_t i = 0; i < n; ++i) {
    const Entry entry = MakeEntry(songs[i], rows[i]);
    if (ShouldEnqueue(entry.source, entry.track_id)) {
      out.push_back(entry);
    }
  }
  return out;
}

}  // namespace StreamingMetadataQueue

#endif
