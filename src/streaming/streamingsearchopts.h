#ifndef STRAWBERRY_STREAMINGSEARCHOPTS_H
#define STRAWBERRY_STREAMINGSEARCHOPTS_H

#include "core/settings.h"
#include "core/song.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingservice.h"
#include "utilities/strutils.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace StreamingSearchOpts {

constexpr char kSearchDelay[] = "searchdelay";
constexpr char kArtistsSearchLimit[] = "artistssearchlimit";
constexpr char kAlbumsSearchLimit[] = "albumssearchlimit";
constexpr char kSongsSearchLimit[] = "songssearchlimit";
constexpr char kFetchAlbums[] = "fetchalbums";
constexpr char kRemoveRemastered[] = "remove_remastered";
constexpr char kAlbumExplicit[] = "album_explicit";
constexpr int kDefaultDelayMs = 1500;
constexpr int kDefaultArtistsLimit = 4;
constexpr int kDefaultAlbumsLimit = 10;
constexpr int kDefaultSongsLimit = 10;
constexpr int kMinQueryLength = 2;
constexpr bool kDefaultFetchAlbums = false;
constexpr bool kDefaultRemoveRemastered = true;
constexpr bool kDefaultAlbumExplicit = false;

inline const char *ConfigureLabel() { return "Configure…"; }

inline const char *SearchForThisLabel() { return "Search for this"; }

inline std::string ConfigureServiceLabel(const std::string &service) {
  if (service.empty()) {
    return ConfigureLabel();
  }
  return std::string("Configure ") + service + "…";
}

inline std::string QueryFromSong(const Song &song, StreamingService::SearchType type) {
  switch (type) {
    case StreamingService::SearchType::Artists: {
      const std::string artist = song.EffectiveAlbumartist();
      return artist.empty() ? song.artist() : artist;
    }
    case StreamingService::SearchType::Albums:
      return song.album().empty() ? song.title() : song.album();
    case StreamingService::SearchType::Songs:
    default:
      return song.title().empty() ? song.PrettyTitle() : song.title();
  }
}

inline std::string QueryFromPrimary(const std::string &primary, const Song &song, StreamingService::SearchType type) {
  if (!primary.empty() && primary != "Loading…") {
    return primary;
  }
  return QueryFromSong(song, type);
}

inline bool CanSearchForThis(const std::string &query) { return StreamingProgress::HasQuery(query); }

inline bool HasSearchLimits(const std::string &group) {
  return group == "Tidal" || group == "Qobuz" || group == "Spotify";
}

inline int DefaultLimitFor(StreamingService::SearchType type) {
  switch (type) {
    case StreamingService::SearchType::Artists:
      return kDefaultArtistsLimit;
    case StreamingService::SearchType::Albums:
      return kDefaultAlbumsLimit;
    case StreamingService::SearchType::Songs:
      return kDefaultSongsLimit;
  }
  return kDefaultSongsLimit;
}

inline const char *LimitKey(StreamingService::SearchType type) {
  switch (type) {
    case StreamingService::SearchType::Artists:
      return kArtistsSearchLimit;
    case StreamingService::SearchType::Albums:
      return kAlbumsSearchLimit;
    case StreamingService::SearchType::Songs:
      return kSongsSearchLimit;
  }
  return kSongsSearchLimit;
}

inline int ClampDelay(int ms) { return ms < 0 ? 0 : ms; }

inline int ClampLimit(int limit, int fallback) { return limit > 0 ? limit : fallback; }

inline bool ShouldDelay(int delay_ms, bool immediate) { return delay_ms > 0 && !immediate; }

inline bool ShouldSearchOnChange(const std::string &query) { return query.size() >= static_cast<size_t>(kMinQueryLength); }

inline int DelayMs(const std::string &group) {
  Settings settings;
  settings.BeginGroup(group);
  return ClampDelay(settings.IntValue(kSearchDelay, HasSearchLimits(group) ? kDefaultDelayMs : 0));
}

inline int LimitFor(const std::string &group, StreamingService::SearchType type) {
  const int fallback = HasSearchLimits(group) ? DefaultLimitFor(type) : 50;
  Settings settings;
  settings.BeginGroup(group);
  return ClampLimit(settings.IntValue(LimitKey(type), fallback), fallback);
}

inline bool FetchAlbumsEnabled(const std::string &group) {
  if (group != "Tidal" && group != "Spotify") {
    return false;
  }
  Settings settings;
  settings.BeginGroup(group);
  return settings.BoolValue(kFetchAlbums, kDefaultFetchAlbums);
}

inline bool ShouldFetchAlbums(bool enabled, StreamingService::SearchType type) {
  return enabled && type == StreamingService::SearchType::Songs;
}

inline bool ShouldFetchAlbums(const std::string &group, StreamingService::SearchType type) {
  return ShouldFetchAlbums(FetchAlbumsEnabled(group), type);
}

inline bool RemoveRemasteredEnabled(const std::string &group) {
  if (!HasSearchLimits(group)) {
    return false;
  }
  Settings settings;
  settings.BeginGroup(group);
  return settings.BoolValue(kRemoveRemastered, kDefaultRemoveRemastered);
}

inline bool AppendExplicitEnabled(const std::string &group) {
  if (group != "Tidal") {
    return false;
  }
  Settings settings;
  settings.BeginGroup(group);
  return settings.BoolValue(kAlbumExplicit, kDefaultAlbumExplicit);
}

inline bool LooksExplicit(const Song &song) {
  return StrUtils::ContainsInsensitive(song.comment(), "explicit") || StrUtils::ContainsInsensitive(song.title(), "explicit") ||
         StrUtils::ContainsInsensitive(song.album(), "explicit");
}

inline void MarkExplicit(Song *song) {
  if (song && LooksExplicit(*song) && song->comment().empty()) {
    song->set_comment("explicit");
  }
}

inline void ApplyTitles(SongList &songs, bool remove_remastered) {
  for (Song &song : songs) {
    MarkExplicit(&song);
    if (!remove_remastered) {
      continue;
    }
    song.set_title(Song::AlbumRemoveDiscMisc(song.title()));
    if (!song.album().empty()) {
      song.set_album(Song::AlbumRemoveDiscMisc(song.album()));
    }
  }
}

inline void AppendExplicit(SongList &songs) {
  for (Song &song : songs) {
    if (!LooksExplicit(song)) {
      continue;
    }
    if (!song.album().empty() && !StrUtils::ContainsInsensitive(song.album(), "explicit")) {
      song.set_album(song.album() + " (Explicit)");
    }
    if (song.song_id().empty() && !song.title().empty() && !StrUtils::ContainsInsensitive(song.title(), "explicit")) {
      song.set_title(song.title() + " (Explicit)");
    }
  }
}

inline SongList Finish(SongList songs, bool remove_remastered, bool append_explicit) {
  ApplyTitles(songs, remove_remastered);
  if (append_explicit) {
    AppendExplicit(songs);
  }
  return songs;
}

inline SongList Finish(const SongList &songs, const std::string &group) {
  return Finish(songs, RemoveRemasteredEnabled(group), AppendExplicitEnabled(group));
}

inline std::vector<std::string> UniqueAlbumIds(const SongList &songs) {
  std::vector<std::string> ids;
  for (const Song &song : songs) {
    if (song.album_id().empty()) {
      continue;
    }
    bool seen = false;
    for (const std::string &id : ids) {
      if (id == song.album_id()) {
        seen = true;
        break;
      }
    }
    if (!seen) {
      ids.push_back(song.album_id());
    }
  }
  return ids;
}

using FetchOneAlbum = std::function<void(const std::string &album_id, StreamingService::SearchCallback done)>;

inline void FetchEachAlbum(const std::vector<std::string> &ids, FetchOneAlbum fetch_one, StreamingService::SearchCallback done,
                           std::function<bool()> still_current = {}, std::function<void(int received, int total)> progress = {}) {
  if (!fetch_one || ids.empty()) {
    if (done) {
      done({});
    }
    return;
  }
  struct State {
    std::vector<std::string> ids;
    FetchOneAlbum fetch_one;
    StreamingService::SearchCallback done;
    std::function<bool()> still_current;
    std::function<void(int received, int total)> progress;
    SongList songs;
    size_t index = 0;
  };
  auto state = std::make_shared<State>();
  state->ids = ids;
  state->fetch_one = std::move(fetch_one);
  state->done = std::move(done);
  state->still_current = std::move(still_current);
  state->progress = std::move(progress);
  auto step = std::make_shared<std::function<void()>>();
  *step = [state, step]() {
    if (state->still_current && !state->still_current()) {
      return;
    }
    if (state->index >= state->ids.size()) {
      if (state->done) {
        state->done(state->songs);
      }
      return;
    }
    if (state->progress) {
      state->progress(static_cast<int>(state->index), static_cast<int>(state->ids.size()));
    }
    const std::string id = state->ids[state->index++];
    state->fetch_one(id, [state, step](const SongList &page) {
      state->songs.insert(state->songs.end(), page.begin(), page.end());
      (*step)();
    });
  };
  (*step)();
}

}  // namespace StreamingSearchOpts

#endif
