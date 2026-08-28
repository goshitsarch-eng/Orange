#ifndef STRAWBERRY_STREAMINGCOVERDOWNLOAD_H
#define STRAWBERRY_STREAMINGCOVERDOWNLOAD_H

#include "core/network.h"
#include "core/settings.h"
#include "core/song.h"
#include "core/standardpaths.h"
#include "streaming/streamingcover.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingservice.h"
#include "utilities/fileutils.h"
#include "utilities/safefilename.h"
#include "utilities/strutils.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace StreamingCoverDownload {

constexpr char kDownloadAlbumCovers[] = "downloadalbumcovers";
constexpr bool kDefaultDownloadAlbumCovers = true;

struct Job {
  std::string album_id;
  std::string url;
  std::string filename;
};

inline bool HasDownloadSetting(const std::string &group) {
  return group == "Tidal" || group == "Qobuz" || group == "Spotify" || group == "Subsonic";
}

inline bool Enabled(const std::string &group) {
  if (!HasDownloadSetting(group)) {
    return false;
  }
  Settings settings;
  settings.BeginGroup(group);
  return settings.BoolValue(kDownloadAlbumCovers, kDefaultDownloadAlbumCovers);
}

inline const char *SourceGroup(Song::Source source) {
  switch (source) {
    case Song::Source::Tidal:
      return "Tidal";
    case Song::Source::Qobuz:
      return "Qobuz";
    case Song::Source::Spotify:
      return "Spotify";
    case Song::Source::Subsonic:
      return "Subsonic";
    default:
      return "";
  }
}

inline std::string FileNameFromUrl(const std::string &url) {
  std::string path = url;
  const auto query = path.find('?');
  if (query != std::string::npos) {
    path = path.substr(0, query);
  }
  const auto slash = path.rfind('/');
  if (slash != std::string::npos && slash + 1 < path.size()) {
    return path.substr(slash + 1);
  }
  return {};
}

inline std::string CacheFilename(const std::string &group, const std::string &album_id, const std::string &url) {
  if (album_id.empty()) {
    return {};
  }
  // The album id and the name taken out of the URL both come from the streaming server, so neither may be
  // trusted to stay inside the cover cache directory.
  const std::string id = SafeFilename::Component(album_id, "cover");
  if (group == "Tidal") {
    const std::string base = FileNameFromUrl(url);
    return base.empty() ? id : id + "-" + SafeFilename::Component(base, "cover");
  }
  return id;
}

inline std::string CachePath(const std::string &filename) {
  if (filename.empty()) {
    return {};
  }
  return FileUtils::Join(StandardPaths::CoverCacheDir(), filename);
}

inline bool CacheReady(const std::string &path) { return !path.empty() && FileUtils::IsFile(path); }

inline std::string Receiving(int count) {
  if (count == 1) {
    return "Receiving album cover for 1 album...";
  }
  return "Receiving album covers for " + std::to_string(count) + " albums...";
}

inline bool NeedsDownload(const Song &song) {
  const std::string url = StreamingCover::CoverUrl(song);
  if (!StreamingCover::IsHttpUrl(url)) {
    return false;
  }
  return !CacheFilename(SourceGroup(song.source()), song.album_id(), url).empty();
}

inline std::vector<Job> UniqueAlbums(const SongList &songs) {
  std::vector<Job> jobs;
  for (const Song &song : songs) {
    if (!NeedsDownload(song)) {
      continue;
    }
    const std::string url = StreamingCover::CoverUrl(song);
    const std::string filename = CacheFilename(SourceGroup(song.source()), song.album_id(), url);
    bool seen = false;
    for (const Job &job : jobs) {
      if (job.album_id == song.album_id() || job.url == url) {
        seen = true;
        break;
      }
    }
    if (!seen) {
      jobs.push_back({song.album_id(), url, filename});
    }
  }
  return jobs;
}

inline bool ShouldDownload(bool enabled, const SongList &songs) { return enabled && !UniqueAlbums(songs).empty(); }

inline bool IsCoverArtId(const std::string &value) {
  return !value.empty() && !StreamingCover::IsHttpUrl(value) && !StreamingCover::IsLocalUrl(value);
}

// Recovers the cover art id from a signed getCoverArt URL.  The signed URL carries the user's credentials,
// so it is the id that gets cached; the URL is rebuilt from current settings whenever it is needed.
inline std::string CoverArtIdFromUrl(const std::string &url) {
  if (url.find("getCoverArt") == std::string::npos) {
    return {};
  }
  const std::string::size_type question = url.find('?');
  if (question == std::string::npos) {
    return {};
  }
  std::string::size_type pos = question + 1;
  while (pos < url.size()) {
    const std::string::size_type amp = url.find('&', pos);
    const std::string part = url.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    if (part.rfind("id=", 0) == 0) {
      gchar *unescaped = g_uri_unescape_string(part.substr(3).c_str(), nullptr);
      std::string value = unescaped ? unescaped : part.substr(3);
      g_free(unescaped);
      return value;
    }
    if (amp == std::string::npos) {
      break;
    }
    pos = amp + 1;
  }
  return {};
}

// The value worth caching: the bare id when one can be recovered, otherwise the URL as it stands (other
// services hand out plain CDN links with nothing secret in them).
inline std::string CacheableCoverArt(const std::string &art_automatic) {
  const std::string id = CoverArtIdFromUrl(art_automatic);
  return id.empty() ? art_automatic : id;
}

// Subsonic's getCoverArt takes an album id just as happily as a track's own cover art id.  Asking for the
// album gives every track on it the same cover, and the same request, instead of one per track.
inline std::string CoverArtIdFor(const Song &song, bool prefer_album_id) {
  if (prefer_album_id && IsCoverArtId(song.album_id())) {
    return song.album_id();
  }
  return song.art_automatic();
}

inline void ApplyCoverArtIds(SongList &songs, const std::function<std::string(const std::string &id)> &url_for,
                             bool prefer_album_id = false) {
  if (!url_for) {
    return;
  }
  for (Song &song : songs) {
    if (!IsCoverArtId(song.art_automatic())) {
      continue;
    }
    song.set_art_automatic(url_for(CoverArtIdFor(song, prefer_album_id)));
  }
}

inline void ApplyLocalCover(SongList &songs, const std::string &album_id, const std::string &path) {
  if (path.empty()) {
    return;
  }
  const std::string uri = FileUtils::UriFromPath(path);
  for (Song &song : songs) {
    if (!album_id.empty() && song.album_id() == album_id) {
      song.set_art_automatic(uri);
    }
  }
}

void AfterList(NetworkAccessManager *network, const std::map<std::string, std::string> &headers, const std::string &group, SongList songs,
               StreamingService::SearchCallback done, std::function<void(const std::string &)> status = {},
               StreamingPage::ProgressCallback progress = {}, StreamingPage::StillCurrent still_current = {});

}  // namespace StreamingCoverDownload

#endif
