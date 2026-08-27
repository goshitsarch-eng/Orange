#include "device/devicecopyrunner.h"

#include "config.h"
#include "core/logging.h"
#include "core/standardpaths.h"
#include "core/taskmanager.h"
#include "device/devicecopyjob.h"
#include "device/devicecopysupported.h"
#include "device/devicemanager.h"
#include "device/gpoddevice.h"
#include "device/mtpdevice.h"
#include "organize/organize.h"
#include "organize/organizecoversource.h"
#include "organize/organizeformat.h"
#include "organize/organizetranscode.h"
#include "transcoder/transcoder.h"
#include "utilities/fileutils.h"

#include <glib.h>
#ifdef HAVE_GIO
#include <glib/gstdio.h>
#endif

namespace {

gboolean DeviceCopyProcessIdle(gpointer data) {
  static_cast<DeviceCopyRunner *>(data)->IdleTick();
  return G_SOURCE_REMOVE;
}

}  // namespace

DeviceCopyRunner::DeviceCopyRunner(TaskManager *task_manager, TagReader *tagreader)
    : task_manager_(task_manager), tagreader_(tagreader) {}

DeviceCopyRunner::~DeviceCopyRunner() {
  if (idle_id_) {
    g_source_remove(idle_id_);
    idle_id_ = 0;
  }
  if (gpod_) {
    gpod_->Finish();
    gpod_.reset();
  }
  mtp_.reset();
  if (task_id_ > 0 && task_manager_) {
    task_manager_->SetTaskFinished(task_id_);
    task_id_ = 0;
  }
}

bool DeviceCopyRunner::Copy(const ConnectedDevice &device, const SongList &songs) {
  Begin(device, songs);
  async_ = false;
  if (task_manager_) {
    task_id_ = task_manager_->StartTask(DeviceCopyJob::TaskName());
    task_manager_->SetTaskBlocksCollectionScans(task_id_);
  }
  while (!finished_) {
    ProcessSome();
  }
  return copied_ > 0 && errors_.size() < songs_.size();
}

void DeviceCopyRunner::Begin(const ConnectedDevice &device, const SongList &songs) {
  device_ = device;
  songs_ = songs;
  errors_.clear();
  next_ = 0;
  copied_ = 0;
  cancelled_ = false;
  finished_ = false;
  async_ = false;
  session_open_ = false;
  supported_filetypes_.clear();
  mtp_.reset();
  gpod_.reset();
}

void DeviceCopyRunner::StartAsync(const ConnectedDevice &device, const SongList &songs) {
  Begin(device, songs);
  async_ = true;
  if (task_manager_) {
    task_id_ = task_manager_->StartTask(DeviceCopyJob::TaskName());
    task_manager_->SetTaskBlocksCollectionScans(task_id_);
  }
  ScheduleIdle();
}

void DeviceCopyRunner::Cancel() { cancelled_ = true; }

void DeviceCopyRunner::IdleTick() {
  idle_id_ = 0;
  ProcessSome();
}

void DeviceCopyRunner::ScheduleIdle() {
  if (idle_id_) {
    return;
  }
  idle_id_ = g_idle_add(DeviceCopyProcessIdle, this);
}

bool DeviceCopyRunner::OpenSession() {
  if (session_open_) {
    return true;
  }
#ifdef HAVE_MTP
  if (device_.backend == "mtp") {
    mtp_ = std::make_unique<MtpCopySession>();
    session_open_ = mtp_->Open(DeviceCopyJob::MtpSerial(device_.unique_id));
    if (session_open_) {
      supported_filetypes_ = DeviceCopySupported::ForCopy(device_.backend, mtp_->SupportedFiletypes());
    }
    return session_open_;
  }
#endif
#ifdef HAVE_GPOD
  if (device_.backend == "gpod") {
    gpod_ = std::make_unique<GPodCopySession>();
    session_open_ = gpod_->Open(device_.mount_path);
    if (session_open_) {
      supported_filetypes_ = DeviceCopySupported::ForCopy(device_.backend, {});
    }
    return session_open_;
  }
#endif
  session_open_ = true;
  supported_filetypes_ = DeviceCopySupported::ForCopy(device_.backend, {});
  return true;
}

Song DeviceCopyRunner::PrepareSong(const Song &song) {
  const std::vector<Song::FileType> supported =
      supported_filetypes_.empty() ? DeviceCopySupported::ForCopy(device_.backend, {}) : supported_filetypes_;
  const Song::FileType dest_type = OrganizeTranscode::Check(song.filetype(), transcode_mode_, transcode_format_, supported);
  if (dest_type == Song::FileType::Unknown || !OrganizeTranscode::CanTranscode(dest_type)) {
    return song;
  }
  const std::string src = FileUtils::PathFromUri(song.url());
  const std::string temp = OrganizeTranscode::FiddleExtension(
      FileUtils::Join(StandardPaths::CacheDir(), "device-" + FileUtils::BaseName(src)), OrganizeTranscode::ExtensionForFileType(dest_type));
  Transcoder transcoder;
  if (!transcoder.TranscodeFile(song, temp, OrganizeTranscode::FormatFromFileType(dest_type))) {
    return Song();
  }
  Song copy = song;
  copy.set_url(FileUtils::UriFromPath(temp));
  copy.set_filetype(dest_type);
  copy.set_basefilename(FileUtils::BaseName(temp));
  return copy;
}

bool DeviceCopyRunner::CopyOnePrepared(const Song &song) {
#ifdef HAVE_MTP
  if (device_.backend == "mtp") {
    return mtp_ && mtp_->CopyOne(song, [this](float fraction) {
      if (task_manager_ && task_id_ > 0) {
        const int total = static_cast<int>(songs_.size());
        task_manager_->SetTaskProgress(task_id_, DeviceCopyJob::ScaledProgress(next_, fraction, total), DeviceCopyJob::ScaledProgressMax(total));
      }
    });
  }
#endif
#ifdef HAVE_GPOD
  if (device_.backend == "gpod") {
    std::string cover;
    if (albumcover_) {
      cover = OrganizeCoverSource::ForSong(song, tagreader_, FileUtils::Join(StandardPaths::CacheDir(), "device-cover.bin"));
    }
    return gpod_ && gpod_->CopyOne(song, playlist_, cover);
  }
#endif
#ifdef HAVE_GIO
  if (device_.mount_path.empty()) {
    return false;
  }
  const std::string music = DeviceManager::MusicPath(device_);
  g_mkdir_with_parents(music.c_str(), 0755);
  OrganizeFormat format("%albumartist/%album/{%track - }%title");
  Organize::Options options;
  options.overwrite = overwrite_;
  options.albumcover = albumcover_;
  options.move = move_;
  options.tagreader = tagreader_;
  options.cover_cache_path = FileUtils::Join(StandardPaths::CacheDir(), "device-cover.bin");
  options.transcode_mode = MusicStorage::TranscodeMode::Transcode_Never;
  options.playlist = playlist_;
  class Organize organize;
  return organize.Copy({song}, music, format, options).empty();
#else
  (void)song;
  return false;
#endif
}

void DeviceCopyRunner::Complete() {
#ifdef HAVE_GPOD
  if (gpod_) {
    if (!gpod_->Finish() && copied_ > 0) {
      LogWarning("Writing iPod database failed after copying %d songs", copied_);
    }
    gpod_.reset();
  }
#endif
  mtp_.reset();
  session_open_ = false;
  if (task_id_ > 0 && task_manager_) {
    task_manager_->SetTaskFinished(task_id_);
    task_id_ = 0;
  }
  finished_ = true;
  Finished.Emit(copied_ > 0 && errors_.size() < songs_.size());
}

void DeviceCopyRunner::ProcessSome() {
  if (finished_) {
    return;
  }
  const int total = static_cast<int>(songs_.size());
  if (!session_open_ && !OpenSession()) {
    errors_ = songs_;
    next_ = total;
    Complete();
    return;
  }
  if (!DeviceCopyJob::ShouldProcessBatch(cancelled_)) {
    if (DeviceCopyJob::ShouldFinish(next_, total, cancelled_)) {
      Complete();
    }
    return;
  }
  int processed = 0;
  while (processed < DeviceCopyJob::kBatchSize && next_ < total) {
    const Song prepared = PrepareSong(songs_[static_cast<size_t>(next_)]);
    if (!prepared.is_valid() || !CopyOnePrepared(prepared)) {
      errors_.push_back(songs_[static_cast<size_t>(next_)]);
    } else {
      ++copied_;
      if (move_) {
        FileUtils::Remove(FileUtils::PathFromUri(songs_[static_cast<size_t>(next_)].url()));
      }
    }
    ++next_;
    ++processed;
  }
  if (task_manager_ && task_id_ > 0) {
    task_manager_->SetTaskProgress(task_id_, DeviceCopyJob::ScaledProgress(next_, 0.0f, total), DeviceCopyJob::ScaledProgressMax(total));
  }
  if (DeviceCopyJob::ShouldFinish(next_, total, cancelled_)) {
    Complete();
    return;
  }
  if (DeviceCopyJob::ShouldScheduleNext(next_, total, cancelled_, async_)) {
    ScheduleIdle();
  }
}
