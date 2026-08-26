#include "smartplaylists/playlistquerygenerator.h"

#include "collection/collectionbackend.h"

#include <algorithm>

PlaylistQueryGenerator::PlaylistQueryGenerator(const std::string &name, const SmartPlaylistSearch &search, bool dynamic)
    : search_(search), dynamic_(dynamic) {
  set_name(name);
}

void PlaylistQueryGenerator::Load(const SmartPlaylistSearch &search) { search_ = search; }

void PlaylistQueryGenerator::Load(const std::string &data) { SmartPlaylistSearch::Parse(data, &search_); }

std::string PlaylistQueryGenerator::Save() const { return search_.Serialize(); }

SongList PlaylistQueryGenerator::Generate() {
  SongList songs;
  if (collection_backend_) {
    songs = search_.Search(collection_backend_);
  }
  previous_urls_.clear();
  for (const Song &song : songs) {
    previous_urls_.push_back(song.url());
  }
  return songs;
}

SongList PlaylistQueryGenerator::GenerateMore(int count) {
  SmartPlaylistSearch more = search_;
  if (count > 0) {
    more.limit = count + static_cast<int>(previous_urls_.size());
  }
  SongList songs = collection_backend_ ? more.Search(collection_backend_) : SongList{};
  SongList fresh;
  for (const Song &song : songs) {
    if (std::find(previous_urls_.begin(), previous_urls_.end(), song.url()) != previous_urls_.end()) {
      continue;
    }
    fresh.push_back(song);
    previous_urls_.push_back(song.url());
    if (count > 0 && static_cast<int>(fresh.size()) >= count) {
      break;
    }
  }
  return fresh;
}
