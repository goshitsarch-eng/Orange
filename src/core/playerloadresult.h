#ifndef STRAWBERRY_PLAYERLOADRESULT_H
#define STRAWBERRY_PLAYERLOADRESULT_H

#include "core/song.h"
#include "core/urlhandler.h"

#include <algorithm>
#include <string>
#include <vector>

namespace PlayerLoadResult {

enum class Target { None, Current, Next };

inline Target MatchMediaUrl(const std::string &media_url, const std::string &current_url, const std::string &next_url) {
  if (!media_url.empty() && media_url == next_url && media_url != current_url) {
    return Target::Next;
  }
  if (media_url.empty() || media_url == current_url) {
    return Target::Current;
  }
  if (media_url == next_url) {
    return Target::Next;
  }
  return Target::None;
}

inline bool ShouldDeferEngineStart(UrlHandler::LoadResult::Type type) {
  return type == UrlHandler::LoadResult::Type::WillLoadAsynchronously || type == UrlHandler::LoadResult::Type::Async;
}

inline bool ShouldAdvanceOnNoMoreTracks(UrlHandler::LoadResult::Type type) {
  return type == UrlHandler::LoadResult::Type::NoMoreTracks;
}

inline bool ShouldTreatAsError(UrlHandler::LoadResult::Type type) { return type == UrlHandler::LoadResult::Type::Error; }

inline bool ShouldPreloadResolved(Target target, bool current_is_module) { return target == Target::Next && !current_is_module; }

inline bool LoadingAsyncContains(const std::vector<std::string> &urls, const std::string &url) {
  return std::find(urls.begin(), urls.end(), url) != urls.end();
}

inline void LoadingAsyncInsert(std::vector<std::string> *urls, const std::string &url) {
  if (!urls || url.empty() || LoadingAsyncContains(*urls, url)) {
    return;
  }
  urls->push_back(url);
}

inline void LoadingAsyncErase(std::vector<std::string> *urls, const std::string &url) {
  if (!urls || url.empty()) {
    return;
  }
  urls->erase(std::remove(urls->begin(), urls->end(), url), urls->end());
}

inline void Apply(Song *song, const UrlHandler::LoadResult &result) {
  if (!song) {
    return;
  }
  if (!result.stream_url.empty()) {
    song->set_stream_url(result.stream_url);
  }
  if (result.filetype != Song::FileType::Unknown) {
    song->set_filetype(result.filetype);
  }
  if (result.samplerate > 0) {
    song->set_samplerate(result.samplerate);
  }
  if (result.bit_depth > 0) {
    song->set_bitdepth(result.bit_depth);
  }
  if (result.duration > 0) {
    song->set_length_nanosec(result.duration);
  }
  if (result.song.is_valid()) {
    if (!result.song.title().empty()) {
      song->set_title(result.song.title());
    }
    if (!result.song.artist().empty()) {
      song->set_artist(result.song.artist());
    }
    if (!result.song.album().empty()) {
      song->set_album(result.song.album());
    }
    if (!result.song.genre().empty()) {
      song->set_genre(result.song.genre());
    }
  }
}

}  // namespace PlayerLoadResult

#endif  // STRAWBERRY_PLAYERLOADRESULT_H
