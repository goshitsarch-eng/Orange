#ifndef STRAWBERRY_ALBUMCOVERMANAGERLIST_H
#define STRAWBERRY_ALBUMCOVERMANAGERLIST_H

#include "core/song.h"

#include <string>
#include <vector>

class AlbumCoverManagerList {
 public:
  enum class HideCovers { None, WithCovers, WithoutCovers };

  static constexpr char kAllArtists[] = "";
  static constexpr char kVariousArtists[] = "Various artists";

  struct Album {
    std::string artist;
    std::string album;
    Song song;
    bool has_cover = false;
    bool various = false;
  };

  void SetSongs(const SongList &songs);
  void SetCoverFlag(const std::string &artist, const std::string &album, bool has_cover);

  const std::vector<Album> &albums() const { return albums_; }
  int album_count() const { return static_cast<int>(albums_.size()); }
  int with_cover_count() const;
  int without_cover_count() const { return album_count() - with_cover_count(); }

  std::vector<std::string> Artists() const;
  std::vector<Album> Filtered(const std::string &artist, HideCovers hide, const std::string &filter) const;
  static bool ShouldHide(const Album &album, HideCovers hide, const std::string &filter);
  static SongList SongsInAlbum(const SongList &all, const Album &album);
  static std::string Key(const std::string &artist, const std::string &album);
  static bool IsVarious(const Song &song);

 private:
  std::vector<Album> albums_;
};

#endif
