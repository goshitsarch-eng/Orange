#ifndef STRAWBERRY_COLLECTIONBACKEND_H
#define STRAWBERRY_COLLECTIONBACKEND_H

#include "collection/collectiondirectory.h"
#include "collection/collectionfilteroptions.h"
#include "core/database.h"
#include "core/signal.h"
#include "core/song.h"

#include <string>
#include <vector>

class CollectionBackend {
 public:
  explicit CollectionBackend(Database *database);

  std::vector<CollectionDirectory> Directories() const;
  int AddDirectory(const std::string &path, bool subdirs = true);
  void RemoveDirectory(int id);

  SongList Songs(const std::string &filter = {}) const;
  SongList Songs(const CollectionFilterOptions &options) const;
  Song SongById(int id) const;
  Song SongByUrl(const std::string &url) const;
  int AddOrUpdateSong(const Song &song);
  void DeleteSongsInDirectory(int directory_id);
  int DeleteSongsBySource(Song::Source source);
  void IncrementPlayCount(int song_id);
  void IncrementSkipCount(int song_id);
  void ResetPlayStatistics(int song_id);
  void SetRating(int song_id, float rating);
  void SetUnavailable(int song_id, bool unavailable);
  int ForceCompilation(const SongList &songs, bool on);
  int MarkMissingUnavailable(int directory_id, const std::vector<std::string> &seen_urls);
  int SongCount() const;

  Signal<SongList> SongsDiscovered;
  Signal<SongList> SongsDeleted;
  Signal<> DirectoryAdded;
  Signal<> DirectoryDeleted;

 private:
  Song SongFromQuery(const SqlQuery &query) const;

  Database *database_;
};

#endif  // STRAWBERRY_COLLECTIONBACKEND_H
