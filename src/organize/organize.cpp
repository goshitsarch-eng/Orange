#include "organize/organize.h"

#include "collection/collectionbackend.h"
#include "core/filesystemmusicstorage.h"
#include "core/standardpaths.h"
#include "core/taskmanager.h"
#include "organize/organizecoversource.h"
#include "organize/organizejob.h"
#include "organize/organizepathnotify.h"
#include "organize/organizetranscode.h"
#include "transcoder/transcoder.h"
#include "utilities/fileutils.h"

#include <glib.h>
#include <glib/gstdio.h>

namespace {

gboolean OrganizeProcessIdle(gpointer data) {
  static_cast<Organize *>(data)->IdleTick();
  return G_SOURCE_REMOVE;
}

}  // namespace

Organize::Organize(TaskManager *task_manager) : task_manager_(task_manager) {}

Organize::~Organize() {
  if (idle_id_) {
    g_source_remove(idle_id_);
    idle_id_ = 0;
  }
  if (task_id_ > 0 && task_manager_) {
    task_manager_->SetTaskFinished(task_id_);
    task_id_ = 0;
  }
}

std::string Organize::CoverPathForSong(const Song &song) {
  for (const std::string &art : {song.art_manual(), song.art_automatic()}) {
    const std::string path = FileUtils::PathFromUri(art);
    if (!path.empty() && FileUtils::IsFile(path)) {
      return path;
    }
  }
  const std::string dir = FileUtils::DirName(FileUtils::PathFromUri(song.url()));
  if (dir.empty()) {
    return {};
  }
  for (const char *name : {"cover.jpg", "cover.png", "folder.jpg", "front.jpg", "album.jpg"}) {
    const std::string path = FileUtils::Join(dir, name);
    if (FileUtils::IsFile(path)) {
      return path;
    }
  }
  return {};
}

std::vector<Organize::Error> Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move) {
  Options options;
  options.move = move;
  return Copy(songs, destination, format, options);
}

std::vector<Organize::Error> Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format,
                                            const Options &options) {
  Begin(songs, destination, format, options);
  async_ = false;
  while (!finished_) {
    ProcessSome();
  }
  return errors_;
}

void Organize::Begin(const SongList &songs, const std::string &destination, const OrganizeFormat &format, const Options &options) {
  destination_ = destination;
  format_ = format;
  options_ = options;
  next_ = 0;
  cancelled_ = false;
  finished_ = false;
  async_ = false;
  waiting_for_transcode_ = false;
  errors_.clear();
  entries_.clear();
  if (destination.empty()) {
    errors_.push_back({"", "Destination folder is empty"});
    return;
  }
  entries_ = OrganizePreview::Compute(songs, format, options.transcode_mode, options.transcode_format, options.supported_filetypes);
}

void Organize::Start(const SongList &songs, const std::string &destination, const OrganizeFormat &format, const Options &options) {
  Begin(songs, destination, format, options);
  async_ = true;
  if (task_manager_) {
    task_id_ = task_manager_->StartTask(OrganizeJob::TaskName());
    task_manager_->SetTaskBlocksCollectionScans(task_id_);
  }
  ScheduleIdle();
}

void Organize::Cancel() { cancelled_ = true; }

void Organize::IdleTick() {
  idle_id_ = 0;
  ProcessSome();
}

void Organize::ScheduleIdle() {
  if (idle_id_) {
    return;
  }
  idle_id_ = g_idle_add(OrganizeProcessIdle, this);
}

void Organize::Complete() {
  if (task_id_ > 0 && task_manager_) {
    task_manager_->SetTaskFinished(task_id_);
    task_id_ = 0;
  }
  finished_ = true;
  Finished.Emit(this);
}

void Organize::ProcessSome() {
  if (finished_) {
    return;
  }
  const int total = static_cast<int>(entries_.size());
  if (!OrganizeJob::ShouldProcessBatch(cancelled_, waiting_for_transcode_)) {
    if (OrganizeJob::ShouldFinish(next_, total, waiting_for_transcode_, cancelled_)) {
      Complete();
    }
    return;
  }
  int processed = 0;
  while (processed < OrganizeJob::kBatchSize && next_ < total) {
    ProcessOne(entries_[static_cast<size_t>(next_)]);
    ++next_;
    ++processed;
    if (waiting_for_transcode_) {
      break;
    }
  }
  if (task_manager_ && task_id_ > 0) {
    task_manager_->SetTaskProgress(task_id_, OrganizeJob::Progress(next_), OrganizeJob::ProgressMax(total));
  }
  if (OrganizeJob::ShouldFinish(next_, total, waiting_for_transcode_, cancelled_)) {
    Complete();
    return;
  }
  if (OrganizeJob::ShouldScheduleNext(next_, total, waiting_for_transcode_, cancelled_, async_)) {
    ScheduleIdle();
  }
}

void Organize::ProcessOne(const OrganizePreview::Entry &entry) {
  const Song &song = entry.song;
  if (OrganizeJob::ShouldSkipInvalid(song)) {
    return;
  }
  const std::string src = FileUtils::PathFromUri(song.url());
  if (entry.relative_path.empty()) {
    errors_.push_back({song.PrettyTitleWithArtist(), "Filename format produced an empty path"});
    return;
  }
  std::string dest = FileUtils::Join(destination_, entry.relative_path);
  const Song::FileType dest_type =
      OrganizeTranscode::Check(song.filetype(), options_.transcode_mode, options_.transcode_format, options_.supported_filetypes);
  std::string temp;
  std::string copy_src = src;
  if (dest_type != Song::FileType::Unknown && OrganizeTranscode::CanTranscode(dest_type)) {
    temp = OrganizeTranscode::FiddleExtension(FileUtils::Join(StandardPaths::CacheDir(), "organize-" + FileUtils::BaseName(src)),
                                              OrganizeTranscode::ExtensionForFileType(dest_type));
    Transcoder transcoder;
    if (!transcoder.TranscodeFile(song, temp, OrganizeTranscode::FormatFromFileType(dest_type))) {
      errors_.push_back({song.PrettyTitleWithArtist(), "Transcode failed"});
      return;
    }
    copy_src = temp;
  }
  if (src.empty() || !FileUtils::Exists(copy_src)) {
    errors_.push_back({song.PrettyTitleWithArtist(), "Source file is missing"});
    return;
  }
  if (OrganizeJob::ShouldSkipExisting(options_.overwrite, FileUtils::Exists(dest))) {
    errors_.push_back({song.PrettyTitleWithArtist(), "Destination already exists"});
    if (!temp.empty()) {
      FileUtils::Remove(temp);
    }
    return;
  }
  g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
  FilesystemMusicStorage storage(destination_);
  MusicStorage::CopyJob job;
  job.source = copy_src;
  job.destination = dest;
  job.metadata = song;
  job.overwrite = options_.overwrite;
  job.remove_original = options_.move && temp.empty();
  job.albumcover = options_.albumcover;
  job.playlist = options_.playlist;
  if (options_.albumcover) {
    job.cover_source = OrganizeCoverSource::ForSong(song, options_.tagreader, options_.cover_cache_path);
    job.cover_dest = FileUtils::Join(FileUtils::DirName(dest), "cover.jpg");
  }
  std::string error_text;
  if (!storage.CopyToStorage(job, error_text)) {
    errors_.push_back({song.PrettyTitleWithArtist(), error_text.empty() ? ("Could not write " + dest) : error_text});
  } else {
    if (OrganizePathNotify::ShouldNotify(options_.move, song, options_.destination_is_collection) && options_.collection_backend) {
      options_.collection_backend->UpdateSongUrl(song.id(), FileUtils::UriFromPath(dest), options_.collection_directory_id);
    }
    if (options_.move && !temp.empty()) {
      FileUtils::Remove(src);
    }
    if (song.id() > 0) {
      FileCopied.Emit(song.id());
    }
  }
  if (!temp.empty()) {
    FileUtils::Remove(temp);
  }
}
