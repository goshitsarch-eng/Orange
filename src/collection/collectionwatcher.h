#ifndef STRAWBERRY_COLLECTIONWATCHER_H
#define STRAWBERRY_COLLECTIONWATCHER_H

#include "constants/collectionsettings.h"
#include "collection/collectioncuescan.h"
#include "collection/collectiondirectory.h"
#include "collection/collectionrescanreason.h"
#include "collection/collectionscandelay.h"
#include "collection/collectionsubdirectory.h"
#include "core/filesystemwatcherinotify.h"
#include "core/signal.h"
#include "core/song.h"

#include <gio/gio.h>

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class CollectionBackend;
class TagReader;
class TaskManager;

class CollectionWatcher {
 public:
  enum class ScanType { Incremental, Full };

  CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager);
  ~CollectionWatcher();

  void Scan();
  void Scan(ScanType type);
  void ScanDirectory(int directory_id, const std::string &path, bool recursive);
  void Abort();
  bool scanning() const { return scanning_; }
  void StartWatching();
  void StopWatching();
  void SetRescanPaused(bool pause);
  bool rescan_paused() const { return rescan_paused_; }
  bool incremental_queued() const { return queued_incremental_; }
  int last_added() const { return last_added_; }

  static bool NeedsRescan(const Song &existing, int64_t mtime, int64_t filesize) {
    if (!existing.is_valid() || existing.unavailable()) {
      return true;
    }
    return !(mtime > 0 && existing.mtime() == mtime && (filesize < 0 || existing.filesize() == filesize));
  }

  static bool NeedsRescan(const Song &existing, int64_t mtime, int64_t filesize, bool song_tracking, bool ebu_analysis,
                          CollectionCueScan::Change cue) {
    if (NeedsRescan(existing, mtime, filesize) || CollectionCueScan::CueForcesRescan(cue)) {
      return true;
    }
    return CollectionRescanReason::NeedsAnalysisRescan(existing, song_tracking, ebu_analysis);
  }

  Signal<> ScanFinished;

 private:
  struct ExistingInfo {
    int64_t mtime = -1;
    int64_t filesize = -1;
    int64_t beginning = 0;
    int id = -1;
    bool unavailable = false;
    bool valid = false;
    bool compilation_on = false;
    bool compilation_off = false;
    std::string fingerprint;
    std::string url;
    std::string cue_path;
    std::string art_manual;
    bool art_unset = false;
    unsigned playcount = 0;
    unsigned skipcount = 0;
    int64_t lastplayed = -1;
    float rating = -1.0f;
    std::optional<double> ebu_lufs;
    std::optional<double> ebu_range;
  };

  struct ScanJob {
    CollectionWatcher *watcher = nullptr;
    std::shared_ptr<std::atomic<bool>> alive;
    ScanType type = ScanType::Incremental;
    std::vector<CollectionDirectory> directories;
    std::map<std::string, std::vector<ExistingInfo>> existing;
    std::map<int, std::vector<CollectionSubdirectory>> stored_subdirs;
    std::vector<CollectionSubdirectory> updated_subdirs;
    SongList songs;
    std::vector<std::string> seen_urls;
    bool aborted = false;
    int added = 0;
    bool song_tracking = false;
    bool ebu_analysis = false;
    bool overwrite_playcount = false;
    bool overwrite_rating = false;
    int expire_days = CollectionSettings::kDefaultExpireUnavailableSongs;
    std::vector<std::string> cover_filters;
  };

  void ScanPath(int directory_id, const std::string &path, bool recursive, int task_id, int *added);
  void WatchPath(const std::string &path);
  void ScheduleIncremental();
  void ArmRescanTimer();
  void StartPeriodicScan();
  void StartAsyncScan(ScanType type);
  static gpointer ScanThread(gpointer data);
  static gboolean ApplyScanJob(gpointer data);
  static gboolean OnRescanTimeout(gpointer data);
  static gboolean OnPeriodicTimeout(gpointer data);
  void CollectDirectory(ScanJob *job, const CollectionDirectory &directory);
  static Song SongFromExisting(const ExistingInfo &info);
  static const ExistingInfo *FindExisting(const ScanJob *job, const std::string &url, int64_t beginning);
  static const ExistingInfo *FindExistingByFingerprint(const ScanJob *job, const std::string &fingerprint,
                                                       const std::string &new_url);
  static bool SubdirNeedsAnalysis(const ScanJob *job, const std::string &subdir_path);
  static void ApplyAnalysis(Song *song, const ScanJob *job);
  static void MergeFromExisting(Song *song, const ExistingInfo *info, const ScanJob *job);
  void CollectFile(ScanJob *job, const CollectionDirectory &directory, const std::string &entry);

  CollectionBackend *backend_;
  TagReader *tagreader_;
  TaskManager *task_manager_;
  int last_added_ = 0;
  std::atomic<bool> abort_{false};
  std::atomic<bool> scanning_{false};
  std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
  std::vector<GFileMonitor *> monitors_;
  std::unique_ptr<FileSystemWatcherInotify> inotify_watcher_;
  guint rescan_timeout_id_ = 0;
  guint periodic_timeout_id_ = 0;
  bool queued_incremental_ = false;
  bool rescan_paused_ = false;
};

#endif  // STRAWBERRY_COLLECTIONWATCHER_H
