#include "covermanager/albumcovermanagerlist.h"

#include "utilities/strutils.h"

#include <algorithm>
#include <map>

std::string AlbumCoverManagerList::Key(const std::string &artist, const std::string &album) { return artist + "\n" + album; }

bool AlbumCoverManagerList::IsVarious(const Song &song) {
  if (song.compilation()) {
    return true;
  }
  const std::string artist = StrUtils::ToLower(song.EffectiveAlbumartist());
  return artist == "various artists" || artist == "various" || artist == "va";
}

void AlbumCoverManagerList::SetSongs(const SongList &songs) {
  albums_.clear();
  std::map<std::string, Album> by_key;
  for (const Song &song : songs) {
    if (song.album().empty()) {
      continue;
    }
    const std::string artist = song.EffectiveAlbumartist();
    const std::string key = Key(artist, song.album());
    if (by_key.count(key)) {
      continue;
    }
    Album album;
    album.artist = artist;
    album.album = song.album();
    album.song = song;
    album.has_cover = !song.art_manual().empty() || !song.art_automatic().empty() || song.art_embedded();
    album.various = IsVarious(song);
    by_key[key] = album;
  }
  albums_.reserve(by_key.size());
  for (auto &entry : by_key) {
    albums_.push_back(std::move(entry.second));
  }
}

void AlbumCoverManagerList::SetCoverFlag(const std::string &artist, const std::string &album, bool has_cover) {
  const std::string key = Key(artist, album);
  for (Album &entry : albums_) {
    if (Key(entry.artist, entry.album) == key) {
      entry.has_cover = has_cover;
      return;
    }
  }
}

int AlbumCoverManagerList::with_cover_count() const {
  int count = 0;
  for (const Album &album : albums_) {
    if (album.has_cover) {
      ++count;
    }
  }
  return count;
}

std::vector<std::string> AlbumCoverManagerList::Artists() const {
  std::vector<std::string> artists;
  bool various = false;
  for (const Album &album : albums_) {
    if (album.various) {
      various = true;
    }
    if (album.artist.empty()) {
      continue;
    }
    if (std::find(artists.begin(), artists.end(), album.artist) == artists.end()) {
      artists.push_back(album.artist);
    }
  }
  std::sort(artists.begin(), artists.end());
  if (various) {
    artists.insert(artists.begin(), kVariousArtists);
  }
  return artists;
}

bool AlbumCoverManagerList::ShouldHide(const Album &album, HideCovers hide, const std::string &filter) {
  if (hide == HideCovers::WithCovers && album.has_cover) {
    return true;
  }
  if (hide == HideCovers::WithoutCovers && !album.has_cover) {
    return true;
  }
  if (filter.empty()) {
    return false;
  }
  const std::string haystack = StrUtils::ToLower(album.album + " " + album.artist);
  for (const std::string &token : StrUtils::Split(StrUtils::ToLower(filter), ' ')) {
    if (token.empty()) {
      continue;
    }
    if (haystack.find(token) == std::string::npos) {
      return true;
    }
  }
  return false;
}

std::vector<AlbumCoverManagerList::Album> AlbumCoverManagerList::Filtered(const std::string &artist, HideCovers hide,
                                                                         const std::string &filter) const {
  std::vector<Album> visible;
  for (const Album &album : albums_) {
    if (!artist.empty()) {
      if (artist == kVariousArtists) {
        if (!album.various) {
          continue;
        }
      } else if (album.artist != artist) {
        continue;
      }
    }
    if (!ShouldHide(album, hide, filter)) {
      visible.push_back(album);
    }
  }
  return visible;
}

SongList AlbumCoverManagerList::SongsInAlbum(const SongList &all, const Album &album) {
  SongList songs;
  for (const Song &song : all) {
    if (song.album() == album.album && song.EffectiveAlbumartist() == album.artist) {
      songs.push_back(song);
    }
  }
  return songs;
}
