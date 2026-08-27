#ifndef STRAWBERRY_COLLECTIONTAGSAVE_H
#define STRAWBERRY_COLLECTIONTAGSAVE_H

#include "core/song.h"

#include <map>
#include <string>
#include <vector>

namespace CollectionTagSave {

struct Pending {
  Song song;
  bool save_playcount = false;
  bool save_rating = false;
};

inline bool FiletypeNeedsDefer(Song::FileType type) {
  return type == Song::FileType::OggFlac || type == Song::FileType::OggVorbis || type == Song::FileType::OggOpus ||
         type == Song::FileType::MPEG;
}

inline bool ShouldDefer(const Song &song, const std::string &playing_url) {
  return song.is_local_file() && !playing_url.empty() && song.url() == playing_url && FiletypeNeedsDefer(song.filetype());
}

inline void Queue(std::map<std::string, Pending> *pending, const Song &song, bool playcount, bool rating) {
  if (!pending || song.url().empty()) {
    return;
  }
  Pending &slot = (*pending)[song.url()];
  if (slot.song.url().empty()) {
    slot.song = song;
  }
  if (playcount) {
    slot.save_playcount = true;
    slot.song.set_playcount(song.playcount());
  }
  if (rating) {
    slot.save_rating = true;
    slot.song.set_rating(song.rating());
  }
}

inline std::vector<std::string> ReadyToFlush(const std::map<std::string, Pending> &pending, const std::string &playing_url) {
  std::vector<std::string> urls;
  for (const auto &entry : pending) {
    if (entry.first != playing_url) {
      urls.push_back(entry.first);
    }
  }
  return urls;
}

}  // namespace CollectionTagSave

#endif  // STRAWBERRY_COLLECTIONTAGSAVE_H
