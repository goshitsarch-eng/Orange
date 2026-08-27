#include "smartplaylists/playlistquerygenerator.h"

#include "collection/collectionbackend.h"
#include "smartplaylists/smartplaylistgeneratemore.h"

#include <algorithm>

PlaylistQueryGenerator::PlaylistQueryGenerator(const std::string &name, const SmartPlaylistSearch &search, bool dynamic)
    : search_(search), dynamic_(dynamic) {
  set_name(name);
}

void PlaylistQueryGenerator::Load(const SmartPlaylistSearch &search) { search_ = search; }

void PlaylistQueryGenerator::Load(const std::string &data) { SmartPlaylistSearch::Parse(data, &search_); }

std::string PlaylistQueryGenerator::Save() const { return search_.Serialize(); }

void PlaylistQueryGenerator::Remember(const SongList &songs) {
  for (const Song &song : songs) {
    if (song.id() > 0 && std::find(previous_ids_.begin(), previous_ids_.end(), song.id()) == previous_ids_.end()) {
      previous_ids_.push_back(song.id());
    }
    if (!song.url().empty() &&
        std::find(previous_urls_.begin(), previous_urls_.end(), song.url()) == previous_urls_.end()) {
      previous_urls_.push_back(song.url());
    }
  }
}

SongList PlaylistQueryGenerator::Generate() {
  previous_urls_.clear();
  previous_ids_.clear();
  current_pos_ = 0;
  return GenerateMore(0);
}

SongList PlaylistQueryGenerator::GenerateMore(int count) {
  const SmartPlaylistSearch more = SmartPlaylistGenerateMore::Prepare(search_, previous_ids_, current_pos_, count);
  current_pos_ = SmartPlaylistGenerateMore::NextPosition(current_pos_, more.limit, more.sort_random);
  SongList songs = collection_backend_ ? more.Search(collection_backend_) : SongList{};
  SongList fresh = SmartPlaylistGenerateMore::FilterFresh(songs, previous_ids_);
  if (count > 0 && static_cast<int>(fresh.size()) > count) {
    fresh.resize(static_cast<size_t>(count));
  }
  for (const Song &song : fresh) {
    previous_urls_.push_back(song.url());
    if (song.id() > 0) {
      previous_ids_.push_back(song.id());
    }
  }
  previous_ids_ = SmartPlaylistGenerateMore::TrimHistory(previous_ids_, GetDynamicFuture() + GetDynamicHistory());
  return fresh;
}
