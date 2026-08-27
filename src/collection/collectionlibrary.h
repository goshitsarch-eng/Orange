#ifndef STRAWBERRY_COLLECTIONLIBRARY_H
#define STRAWBERRY_COLLECTIONLIBRARY_H

#include "collection/collectionbackend.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectiontagsave.h"
#include "collection/collectionwatcher.h"
#include "core/signal.h"

#include <map>
#include <memory>
#include <string>

class Database;
class TagReader;
class TaskManager;

class CollectionLibrary {
 public:
  CollectionLibrary(Database *database, TaskManager *task_manager, TagReader *tagreader);

  CollectionBackend *backend() const { return backend_.get(); }
  CollectionWatcher *watcher() const { return watcher_.get(); }

  void Init();
  void IncrementalScan();
  void FullScan();
  void AbortScan();
  bool scanning() const;
  void Rescan(const SongList &songs);
  void RescanDirectory(int id);
  void AddDirectory(const std::string &path, bool subdirs = true);
  void RemoveDirectory(int id);
  SongList Songs(const std::string &filter = {}) const;
  SongList Songs(const CollectionFilterOptions &options) const;
  void SyncPlaycountAndRatingToFiles();
  void SyncPlaycountAndRatingToFilesAsync();
  void CurrentSongChanged(const Song &song);
  void Stopped();
  void SongsPlaycountChanged(const SongList &songs, bool save_tags = false);
  void SongsRatingChanged(const SongList &songs, bool save_tags = false);
  void SavePendingPlaycountsAndRatings();
  const std::string &current_song_url() const { return current_song_url_; }
  const std::map<std::string, CollectionTagSave::Pending> &pending_song_saves() const { return pending_song_saves_; }

  Signal<> ScanFinished;

 private:
  Database *database_;
  TaskManager *task_manager_;
  TagReader *tagreader_;
  std::unique_ptr<CollectionBackend> backend_;
  std::unique_ptr<CollectionWatcher> watcher_;
  std::string current_song_url_;
  std::map<std::string, CollectionTagSave::Pending> pending_song_saves_;
};

#endif  // STRAWBERRY_COLLECTIONLIBRARY_H
