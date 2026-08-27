#ifndef STRAWBERRY_COLLECTIONBACKEND_H
#define STRAWBERRY_COLLECTIONBACKEND_H

#include "collection/collectiondirectory.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectionsubdirectory.h"
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
  std::vector<CollectionSubdirectory> SubdirsInDirectory(int directory_id) const;
  void AddOrUpdateSubdirs(int directory_id, const std::vector<CollectionSubdirectory> &subdirs);

  SongList Songs(const std::string &filter = {}) const;
  SongList Songs(const CollectionFilterOptions &options) const;
  Song SongById(int id) const;
  Song SongByUrl(const std::string &url, int64_t beginning_nanosec = -1) const;
  void UpdateCompilations();
  void UpdateSongUrl(int song_id, const std::string &url, int directory_id = -1);
  int AddOrUpdateSong(const Song &song);
  int RetainBeginnings(const std::string &url, const std::vector<int64_t> &beginnings);
  void DeleteSongsInDirectory(int directory_id);
  int DeleteSongsBySource(Song::Source source);
  void IncrementPlayCount(int song_id);
  void IncrementSkipCount(int song_id);
  void ResetPlayStatistics(int song_id);
  void SetRating(int song_id, float rating);
  void SetUnavailable(int song_id, bool unavailable);
  int ForceCompilation(const SongList &songs, bool on);
  int MarkMissingUnavailable(int directory_id, const std::vector<std::string> &seen_urls);
  void UpdateLastSeen(int directory_id);
  int ExpireSongs(int directory_id, int expire_days, int64_t now_sec = 0);
  int SongCount() const;

  Signal<SongList> SongsDiscovered;
  Signal<SongList> SongsDeleted;
  Signal<SongList> SongsStatisticsChanged;
  Signal<SongList> SongsRatingChanged;
  Signal<> DirectoryAdded;
  Signal<> DirectoryDeleted;

 private:
  Song SongFromQuery(const SqlQuery &query) const;

  Database *database_;
};

#endif  // STRAWBERRY_COLLECTIONBACKEND_H
