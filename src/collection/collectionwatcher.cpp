#include "collection/collectionwatcher.h"

#include "config.h"
#include "collection/collectionbackend.h"
#include "collection/collectionartpersist.h"
#include "collection/collectioncuescan.h"
#include "collection/collectiondirectoryart.h"
#include "collection/collectionexpire.h"
#include "collection/collectionfingerprintmatch.h"
#include "collection/collectionrescanreason.h"
#include "collection/collectionunavailablerestore.h"
#include "collection/collectionscanprogress.h"
#include "collection/collectionscandelay.h"
#include "collection/collectionscangates.h"
#include "collection/collectionsubdirectory.h"
#include "constants/collectionsettings.h"
#ifdef _WIN32
#include "core/filesystemwatcherwin.h"
#else
#include "core/filesystemwatcherinotify.h"
#endif
#include "core/logging.h"
#include "core/settings.h"
#include "core/songuserdatamerge.h"
#include "core/taskmanager.h"
#include "playlistparsers/cueparser.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#ifdef HAVE_CHROMAPRINT
#include "engine/chromaprinter.h"
#endif
#ifdef HAVE_EBUR128
#include "engine/ebur128analysis.h"
#endif

#include <map>
#include <set>

CollectionWatcher::CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager)
    : backend_(backend), tagreader_(tagreader), task_manager_(task_manager) {}

CollectionWatcher::~CollectionWatcher() {
  Abort();
  if (alive_) {
    *alive_ = false;
  }
  if (rescan_timeout_id_) {
    g_source_remove(rescan_timeout_id_);
    rescan_timeout_id_ = 0;
  }
  if (periodic_timeout_id_) {
    g_source_remove(periodic_timeout_id_);
    periodic_timeout_id_ = 0;
  }
  StopWatching();
}

void CollectionWatcher::Abort() { abort_ = true; }

void CollectionWatcher::Scan() { Scan(ScanType::Incremental); }

void CollectionWatcher::Scan(ScanType type) {
  if (type == ScanType::Incremental && (rescan_paused_ || CollectionScanGates::ShouldSkipIncremental(task_manager_))) {
    queued_incremental_ = true;
    return;
  }
  if (scanning_) {
    if (type == ScanType::Incremental) {
      queued_incremental_ = true;
    }
    return;
  }
  StartAsyncScan(type);
}

void CollectionWatcher::ScheduleIncremental() {
  if (rescan_paused_ || CollectionScanGates::ShouldSkipIncremental(task_manager_) || scanning_) {
    queued_incremental_ = true;
    return;
  }
  ArmRescanTimer();
}

void CollectionWatcher::SetRescanPaused(bool pause) {
  rescan_paused_ = pause;
  if (!pause && queued_incremental_ && !scanning_) {
    queued_incremental_ = false;
    Scan(ScanType::Incremental);
  }
}

void CollectionWatcher::ArmRescanTimer() {
  if (rescan_paused_ || CollectionScanGates::ShouldSkipIncremental(task_manager_)) {
    queued_incremental_ = true;
    return;
  }
  if (rescan_timeout_id_) {
    g_source_remove(rescan_timeout_id_);
    rescan_timeout_id_ = 0;
  }
  rescan_timeout_id_ = g_timeout_add(CollectionScanDelay::kRescanMs, OnRescanTimeout, this);
}

gboolean CollectionWatcher::OnRescanTimeout(gpointer data) {
  auto *self = static_cast<CollectionWatcher *>(data);
  self->rescan_timeout_id_ = 0;
  self->Scan(ScanType::Incremental);
  return G_SOURCE_REMOVE;
}

void CollectionWatcher::StartPeriodicScan() {
  if (periodic_timeout_id_) {
    return;
  }
  periodic_timeout_id_ = g_timeout_add(CollectionScanDelay::kPeriodicMs, OnPeriodicTimeout, this);
}

gboolean CollectionWatcher::OnPeriodicTimeout(gpointer data) {
  static_cast<CollectionWatcher *>(data)->Scan(ScanType::Incremental);
  return G_SOURCE_CONTINUE;
}

void CollectionWatcher::StartAsyncScan(ScanType type) {
  if (!backend_ || scanning_) {
    return;
  }
  abort_ = false;
  scanning_ = true;
  auto *job = new ScanJob;
  job->watcher = this;
  job->alive = alive_;
  job->type = type;
  job->directories = backend_->Directories();
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  job->song_tracking = settings.BoolValue(CollectionSettings::kSongTracking, CollectionSettings::kDefaultSongTracking);
  job->ebu_analysis =
      settings.BoolValue(CollectionSettings::kSongENUR128LoudnessAnalysis, CollectionSettings::kDefaultSongENUR128LoudnessAnalysis);
  job->overwrite_playcount =
      settings.BoolValue(CollectionSettings::kOverwritePlaycount, CollectionSettings::kDefaultOverwritePlaycount);
  job->overwrite_rating = settings.BoolValue(CollectionSettings::kOverwriteRating, CollectionSettings::kDefaultOverwriteRating);
  job->expire_days =
      settings.IntValue(CollectionSettings::kExpireUnavailableSongs, CollectionSettings::kDefaultExpireUnavailableSongs);
  job->cover_filters = CollectionDirectoryArt::FiltersFromSettings();
#ifndef HAVE_CHROMAPRINT
  job->song_tracking = false;
#endif
#ifndef HAVE_EBUR128
  job->ebu_analysis = false;
#endif
  if (type == ScanType::Incremental) {
    for (const Song &song : backend_->Songs()) {
      ExistingInfo info;
      info.id = song.id();
      info.url = song.url();
      info.mtime = song.mtime();
      info.filesize = song.filesize();
      info.beginning = song.beginning_nanosec();
      info.unavailable = song.unavailable();
      info.valid = song.is_valid();
      info.compilation_on = song.compilation_on();
      info.compilation_off = song.compilation_off();
      info.fingerprint = song.fingerprint();
      info.cue_path = song.cue_path();
      info.art_manual = song.art_manual();
      info.art_unset = song.art_unset();
      info.playcount = song.playcount();
      info.skipcount = song.skipcount();
      info.lastplayed = song.lastplayed();
      info.rating = song.rating();
      info.ebu_lufs = song.ebur128_integrated_loudness_lufs();
      info.ebu_range = song.ebur128_loudness_range_lu();
      job->existing[song.url()].push_back(info);
    }
    for (const CollectionDirectory &directory : job->directories) {
      job->stored_subdirs[directory.id] = backend_->SubdirsInDirectory(directory.id);
    }
  }
  g_thread_unref(g_thread_new("collection-scan", ScanThread, job));
}

gpointer CollectionWatcher::ScanThread(gpointer data) {
  auto *job = static_cast<ScanJob *>(data);
  if (job->watcher) {
    for (const CollectionDirectory &directory : job->directories) {
      if (job->watcher->abort_) {
        job->aborted = true;
        break;
      }
      job->watcher->CollectDirectory(job, directory);
    }
  }
  g_idle_add(ApplyScanJob, job);
  return nullptr;
}

Song CollectionWatcher::SongFromExisting(const ExistingInfo &info) {
  Song song;
  song.set_id(info.id);
  song.set_url(info.url);
  song.set_valid(info.valid);
  song.set_mtime(info.mtime);
  song.set_filesize(info.filesize);
  song.set_beginning_nanosec(info.beginning);
  song.set_unavailable(info.unavailable);
  song.set_fingerprint(info.fingerprint);
  song.set_cue_path(info.cue_path);
  song.set_art_manual(info.art_manual);
  song.set_art_unset(info.art_unset);
  song.set_playcount(info.playcount);
  song.set_skipcount(info.skipcount);
  song.set_lastplayed(info.lastplayed);
  song.set_rating(info.rating);
  song.set_compilation_on(info.compilation_on);
  song.set_compilation_off(info.compilation_off);
  song.set_ebur128_integrated_loudness_lufs(info.ebu_lufs);
  song.set_ebur128_loudness_range_lu(info.ebu_range);
  return song;
}

const CollectionWatcher::ExistingInfo *CollectionWatcher::FindExisting(const ScanJob *job, const std::string &url,
                                                                       int64_t beginning) {
  if (!job) {
    return nullptr;
  }
  auto it = job->existing.find(url);
  if (it == job->existing.end() || it->second.empty()) {
    return nullptr;
  }
  for (const ExistingInfo &info : it->second) {
    if (info.beginning == beginning) {
      return &info;
    }
  }
  return &it->second.front();
}

const CollectionWatcher::ExistingInfo *CollectionWatcher::FindExistingByFingerprint(const ScanJob *job,
                                                                                    const std::string &fingerprint,
                                                                                    const std::string &new_url) {
  if (!job || !CollectionFingerprintMatch::IsUsable(fingerprint)) {
    return nullptr;
  }
  for (const auto &entry : job->existing) {
    if (entry.first == new_url) {
      continue;
    }
    if (!CollectionFingerprintMatch::OldPathGone(entry.first)) {
      continue;
    }
    for (const ExistingInfo &info : entry.second) {
      if (info.fingerprint == fingerprint) {
        return &info;
      }
    }
  }
  return nullptr;
}

bool CollectionWatcher::SubdirNeedsAnalysis(const ScanJob *job, const std::string &subdir_path) {
  if (!job || (!job->song_tracking && !job->ebu_analysis)) {
    return false;
  }
  for (const auto &entry : job->existing) {
    const std::string path = FileUtils::PathFromUri(entry.first);
    if (CollectionSubdirectoryScan::ImmediateParent(path) != subdir_path) {
      continue;
    }
    for (const ExistingInfo &info : entry.second) {
      if (CollectionRescanReason::NeedsAnalysisRescan(SongFromExisting(info), job->song_tracking, job->ebu_analysis)) {
        return true;
      }
    }
  }
  return false;
}

void CollectionWatcher::ApplyAnalysis(Song *song, const ScanJob *job) {
  if (!song || !job) {
    return;
  }
#if !defined(HAVE_CHROMAPRINT) && !defined(HAVE_EBUR128)
  (void)song;
  (void)job;
#endif
#ifdef HAVE_CHROMAPRINT
  if (job->song_tracking && song->fingerprint().empty()) {
    Chromaprinter printer(FileUtils::PathFromUri(song->url()));
    const std::string fingerprint = printer.CreateFingerprint();
    song->set_fingerprint(fingerprint.empty() ? "NONE" : fingerprint);
  }
#endif
#ifdef HAVE_EBUR128
  if (job->ebu_analysis && (!song->ebur128_integrated_loudness_lufs() || !song->ebur128_loudness_range_lu())) {
    if (const auto measures = EBUR128Analysis::Compute(*song)) {
      song->set_ebur128_integrated_loudness_lufs(measures->loudness_lufs);
      song->set_ebur128_loudness_range_lu(measures->range_lu);
    }
  }
#endif
}

void CollectionWatcher::MergeFromExisting(Song *song, const ExistingInfo *info, const ScanJob *job) {
  if (!song || !info || !job) {
    return;
  }
  SongUserDataMerge::Merge(song, SongFromExisting(*info), !job->overwrite_playcount, !job->overwrite_rating);
}

void CollectionWatcher::CollectFile(ScanJob *job, const CollectionDirectory &directory, const std::string &entry) {
  if (!job || !tagreader_) {
    return;
  }
  const std::string url = FileUtils::UriFromPath(entry);
  const std::string cue = CueParser::FindCueFilename(entry);
  const int64_t cue_mtime = CollectionCueScan::CueMtime(cue);
  const int64_t media_mtime = FileUtils::FileMtime(entry);
  const int64_t filesize = FileUtils::FileSize(entry);
  const ExistingInfo *existing_info = FindExisting(job, url, 0);
  Song existing = existing_info ? SongFromExisting(*existing_info) : Song();
  const CollectionCueScan::Change cue_change =
      CollectionCueScan::DetectCueChange(existing.has_cue(), existing.cue_path(), cue, cue_mtime);
  const int64_t effective_mtime = CollectionCueScan::EffectiveMtime(media_mtime, existing.has_cue() || cue_mtime > 0
                                                                                     ? CollectionCueScan::CueMtime(existing.cue_path().empty() ? cue
                                                                                                                                               : existing.cue_path())
                                                                                     : 0);
  const int64_t compare_mtime = effective_mtime > 0 ? effective_mtime : media_mtime;
  if (job->type == ScanType::Incremental &&
      CollectionUnavailableRestore::CanRestoreWithoutRescan(existing, compare_mtime, filesize, job->song_tracking,
                                                            job->ebu_analysis, cue_change)) {
    existing.set_unavailable(false);
    existing.set_url(url);
    existing.set_directory_id(directory.id);
    job->songs.push_back(existing);
    ++job->added;
    return;
  }
  if (job->type == ScanType::Incremental &&
      !NeedsRescan(existing, compare_mtime, filesize, job->song_tracking, job->ebu_analysis, cue_change)) {
    return;
  }
  Song file = tagreader_->ReadFile(entry);
  file.set_source(Song::Source::Collection);
  file.set_directory_id(directory.id);
  if (file.url().empty()) {
    file.set_url(url);
  }
  if (file.url().empty()) {
    return;
  }
  file.set_mtime(CollectionCueScan::EffectiveMtime(media_mtime, cue_mtime));
  file.set_filesize(filesize);
  ApplyAnalysis(&file, job);
  if (!existing_info && CollectionFingerprintMatch::IsUsable(file.fingerprint())) {
    if (const ExistingInfo *moved = FindExistingByFingerprint(job, file.fingerprint(), url)) {
      existing_info = moved;
      existing = SongFromExisting(*moved);
    }
  }

  SongList songs;
  if (cue_mtime > 0 && !cue.empty()) {
    CueParser parser;
    songs = parser.Load(FileUtils::ReadFile(cue), cue);
    CueParser::EnrichFromAudioFile(&songs, file);
    for (Song &song : songs) {
      song.set_source(Song::Source::Collection);
      song.set_directory_id(directory.id);
      song.set_mtime(file.mtime());
      song.set_filesize(file.filesize());
      song.set_fingerprint(file.fingerprint());
      song.set_ebur128_integrated_loudness_lufs(file.ebur128_integrated_loudness_lufs());
      song.set_ebur128_loudness_range_lu(file.ebur128_loudness_range_lu());
      song.set_cue_path(cue);
    }
  }
  if (songs.empty()) {
    file.set_cue_path({});
    songs.push_back(file);
  }
  const std::string art = CollectionDirectoryArt::ArtForDirectory(FileUtils::DirName(entry), job->cover_filters);
  const std::string art_uri = art.empty() ? std::string() : FileUtils::UriFromPath(art);
  for (Song &song : songs) {
    const ExistingInfo *match = FindExisting(job, song.url(), song.beginning_nanosec());
    if (!match && existing_info) {
      match = existing_info;
    }
    const Song prior = match ? SongFromExisting(*match) : Song();
    if (CollectionArtPersist::ShouldApplyAutomaticArt(prior, art_uri)) {
      song.set_art_automatic(art_uri);
    } else {
      song.set_art_automatic(CollectionArtPersist::ArtAutomaticForUpdate(prior, song.art_automatic()));
    }
    if (prior.id() > 0) {
      song.set_id(prior.id());
    }
    MergeFromExisting(&song, match, job);
    job->songs.push_back(song);
    ++job->added;
  }
}

void CollectionWatcher::CollectDirectory(ScanJob *job, const CollectionDirectory &directory) {
  const std::vector<std::string> entries =
      directory.subdirs ? FileUtils::ListDirectoryRecursive(directory.path) : FileUtils::ListDirectory(directory.path);
  const std::vector<CollectionSubdirectory> stored = job->stored_subdirs.count(directory.id) ? job->stored_subdirs[directory.id]
                                                                                             : std::vector<CollectionSubdirectory>{};
  std::map<std::string, int64_t> seen_subdirs;
  for (const std::string &entry : entries) {
    if (abort_) {
      job->aborted = true;
      return;
    }
    if (FileUtils::IsDirectory(entry) || !Song::IsAudioFile(entry)) {
      continue;
    }
    const std::string parent = CollectionSubdirectoryScan::ImmediateParent(entry);
    const std::string subdir_path = parent.empty() ? directory.path : parent;
    const int64_t dir_mtime = FileUtils::FileMtime(subdir_path);
    seen_subdirs[subdir_path] = dir_mtime;
    job->seen_urls.push_back(FileUtils::UriFromPath(entry));
    const bool force = SubdirNeedsAnalysis(job, subdir_path);
    if (job->type == ScanType::Incremental &&
        CollectionSubdirectoryScan::ShouldSkip(CollectionSubdirectoryScan::StoredMtime(stored, subdir_path), dir_mtime, force)) {
      continue;
    }
    CollectFile(job, directory, entry);
    if (task_manager_ && (job->added % 25) == 0) {
      // Progress is applied on the main thread after the walk finishes.
    }
  }
  for (const auto &entry : seen_subdirs) {
    CollectionSubdirectory subdir;
    subdir.directory_id = directory.id;
    subdir.path = entry.first;
    subdir.mtime = entry.second;
    job->updated_subdirs.push_back(subdir);
  }
}

gboolean CollectionWatcher::ApplyScanJob(gpointer data) {
  std::unique_ptr<ScanJob> job(static_cast<ScanJob *>(data));
  if (!job->alive || !*job->alive || !job->watcher) {
    return G_SOURCE_REMOVE;
  }
  CollectionWatcher *self = job->watcher;
  const int task_id = self->task_manager_ ? self->task_manager_->StartTask(job->type == ScanType::Full ? "Full collection scan" : "Scanning collection") : 0;
  self->last_added_ = 0;
  const int progress_max = CollectionScanProgress::Total(static_cast<int>(job->songs.size()));
  if (self->task_manager_ && task_id) {
    self->task_manager_->SetTaskProgress(task_id, 0, progress_max);
  }
  if (self->backend_) {
    std::map<std::string, std::vector<int64_t>> beginnings;
    for (const Song &song : job->songs) {
      self->backend_->AddOrUpdateSong(song);
      beginnings[song.url()].push_back(song.beginning_nanosec());
      ++self->last_added_;
      if (self->task_manager_ && task_id && CollectionScanProgress::ShouldReport(self->last_added_)) {
        self->task_manager_->SetTaskProgress(task_id, self->last_added_, progress_max);
      }
    }
    for (const auto &entry : beginnings) {
      self->backend_->RetainBeginnings(entry.first, entry.second);
    }
    Settings settings;
    settings.BeginGroup(CollectionSettings::kSettingsGroup);
    const bool mark_unavailable =
        job->type == ScanType::Full && !job->aborted &&
        settings.BoolValue(CollectionSettings::kMarkSongsUnavailable, CollectionSettings::kDefaultMarkSongsUnavailable);
    if (mark_unavailable) {
      std::set<int> directories;
      for (const CollectionDirectory &directory : job->directories) {
        directories.insert(directory.id);
      }
      for (int directory_id : directories) {
        self->backend_->MarkMissingUnavailable(directory_id, job->seen_urls);
      }
    }
    for (const CollectionDirectory &directory : job->directories) {
      self->backend_->UpdateLastSeen(directory.id);
      if (job->expire_days > 0) {
        self->backend_->ExpireSongs(directory.id, job->expire_days);
      }
    }
    self->backend_->UpdateCompilations();
    std::map<int, std::vector<CollectionSubdirectory>> by_directory;
    for (const CollectionSubdirectory &subdir : job->updated_subdirs) {
      by_directory[subdir.directory_id].push_back(subdir);
    }
    for (const auto &entry : by_directory) {
      self->backend_->AddOrUpdateSubdirs(entry.first, entry.second);
    }
  }
  if (self->task_manager_ && task_id) {
    self->task_manager_->SetTaskProgress(task_id, self->last_added_, progress_max);
    self->task_manager_->SetTaskFinished(task_id);
  }
  self->scanning_ = false;
  self->ScanFinished.Emit();
  if (CollectionScanDelay::ShouldRunAfterFinish(self->queued_incremental_)) {
    self->queued_incremental_ = false;
    self->ScheduleIncremental();
  }
  return G_SOURCE_REMOVE;
}

void CollectionWatcher::StopWatching() {
  if (periodic_timeout_id_) {
    g_source_remove(periodic_timeout_id_);
    periodic_timeout_id_ = 0;
  }
  for (GFileMonitor *monitor : monitors_) {
    g_file_monitor_cancel(monitor);
    g_object_unref(monitor);
  }
  monitors_.clear();
  if (native_watcher_) {
    native_watcher_->Clear();
  }
}

void CollectionWatcher::WatchPath(const std::string &path) {
  if (!native_watcher_) {
#ifdef _WIN32
    native_watcher_ = std::make_unique<FileSystemWatcherWin>();
#else
    native_watcher_ = std::make_unique<FileSystemWatcherInotify>();
#endif
    native_watcher_->PathChanged.Connect([this](const std::string &) { ScheduleIncremental(); });
  }
  native_watcher_->AddPath(path);
  GFile *file = g_file_new_for_path(path.c_str());
  GFileMonitor *monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
  g_object_unref(file);
  if (!monitor) {
    return;
  }
  g_signal_connect(monitor, "changed", G_CALLBACK(+[](GFileMonitor *, GFile *, GFile *, GFileMonitorEvent event, gpointer data) {
                     if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT || event == G_FILE_MONITOR_EVENT_CREATED ||
                         event == G_FILE_MONITOR_EVENT_DELETED) {
                       static_cast<CollectionWatcher *>(data)->ScheduleIncremental();
                     }
                   }),
                   this);
  monitors_.push_back(monitor);
}

void CollectionWatcher::StartWatching() {
  StopWatching();
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  if (!settings.BoolValue(CollectionSettings::kMonitor, CollectionSettings::kDefaultMonitor) || !backend_) {
    return;
  }
  for (const CollectionDirectory &directory : backend_->Directories()) {
    WatchPath(directory.path);
  }
  StartPeriodicScan();
}

void CollectionWatcher::ScanDirectory(int directory_id, const std::string &path, bool recursive) {
  int added = 0;
  const int task_id = task_manager_ ? task_manager_->StartTask("Scanning " + path) : 0;
  ScanPath(directory_id, path, recursive, task_id, &added);
  last_added_ = added;
  if (task_manager_ && task_id) {
    task_manager_->SetTaskFinished(task_id);
  }
}

void CollectionWatcher::ScanPath(int directory_id, const std::string &path, bool recursive, int task_id, int *added) {
  ScanJob job;
  job.watcher = this;
  job.type = ScanType::Full;
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  job.song_tracking = settings.BoolValue(CollectionSettings::kSongTracking, CollectionSettings::kDefaultSongTracking);
  job.ebu_analysis =
      settings.BoolValue(CollectionSettings::kSongENUR128LoudnessAnalysis, CollectionSettings::kDefaultSongENUR128LoudnessAnalysis);
  job.overwrite_playcount =
      settings.BoolValue(CollectionSettings::kOverwritePlaycount, CollectionSettings::kDefaultOverwritePlaycount);
  job.overwrite_rating = settings.BoolValue(CollectionSettings::kOverwriteRating, CollectionSettings::kDefaultOverwriteRating);
#ifndef HAVE_CHROMAPRINT
  job.song_tracking = false;
#endif
#ifndef HAVE_EBUR128
  job.ebu_analysis = false;
#endif
  if (backend_) {
    for (const Song &song : backend_->Songs()) {
      ExistingInfo info;
      info.id = song.id();
      info.url = song.url();
      info.mtime = song.mtime();
      info.filesize = song.filesize();
      info.beginning = song.beginning_nanosec();
      info.unavailable = song.unavailable();
      info.valid = song.is_valid();
      info.compilation_on = song.compilation_on();
      info.compilation_off = song.compilation_off();
      info.fingerprint = song.fingerprint();
      info.cue_path = song.cue_path();
      info.art_manual = song.art_manual();
      info.art_unset = song.art_unset();
      info.playcount = song.playcount();
      info.skipcount = song.skipcount();
      info.lastplayed = song.lastplayed();
      info.rating = song.rating();
      info.ebu_lufs = song.ebur128_integrated_loudness_lufs();
      info.ebu_range = song.ebur128_loudness_range_lu();
      job.existing[song.url()].push_back(info);
    }
  }
  CollectionDirectory directory;
  directory.id = directory_id;
  directory.path = path;
  for (const std::string &entry : FileUtils::ListDirectory(path)) {
    if (abort_) {
      return;
    }
    if (FileUtils::IsDirectory(entry)) {
      if (recursive) {
        ScanPath(directory_id, entry, true, task_id, added);
      }
      continue;
    }
    if (!Song::IsAudioFile(entry)) {
      continue;
    }
    CollectFile(&job, directory, entry);
  }
  std::map<std::string, std::vector<int64_t>> beginnings;
  for (const Song &song : job.songs) {
    if (backend_) {
      backend_->AddOrUpdateSong(song);
      beginnings[song.url()].push_back(song.beginning_nanosec());
    }
    ++(*added);
    if (task_manager_ && task_id && CollectionScanProgress::ShouldReport(*added)) {
      task_manager_->SetTaskProgress(task_id, *added, CollectionScanProgress::Total(static_cast<int>(job.songs.size())));
    }
  }
  if (backend_) {
    for (const auto &entry : beginnings) {
      backend_->RetainBeginnings(entry.first, entry.second);
    }
    backend_->UpdateLastSeen(directory_id);
  }
}
