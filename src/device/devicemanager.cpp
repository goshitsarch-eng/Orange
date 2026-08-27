#include "device/devicemanager.h"

#include "device/deviceerror.h"

#include "config.h"
#include "core/logging.h"
#include "core/standardpaths.h"
#include "core/database.h"
#include "core/filesystemmusicstorage.h"
#include "device/devicedatabasebackend.h"
#include "device/cddadevice.h"
#include "device/cddalister.h"
#include "device/cddasongloader.h"
#include "device/filesystemdevice.h"
#include "device/giolister.h"
#include "device/udisks2lister.h"
#include "device/devicecopyrunner.h"
#include "device/devicecopyrefresh.h"
#include "device/devicecopysupported.h"
#include "device/gpoddevice.h"
#include "device/gpodloader.h"
#include "device/mtpconnection.h"
#include "device/mtpdevice.h"
#include "device/mtploader.h"
#include "organize/organize.h"
#include "organize/organizeformat.h"
#include "organize/organizetranscode.h"
#include "transcoder/transcoder.h"
#include "utilities/fileutils.h"
#include <glib.h>
#ifdef HAVE_GIO
#include <gio/gio.h>
#include <glib/gstdio.h>
#endif
#ifdef HAVE_MTP
#include <libmtp.h>
#include <cstdlib>
#endif
#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#endif
#include "device/cddahelpers.h"
#include "device/cddadiscchange.h"
#include "device/devicescanprogress.h"
#include "core/taskmanager.h"

#include <algorithm>
#include <functional>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>

DeviceManager::DeviceManager(Database *database, TaskManager *task_manager)
    : task_manager_(task_manager),
      url_handler_(std::make_unique<DeviceUrlHandler>(this)),
      device_db_(database ? std::make_unique<DeviceDatabaseBackend>(database) : nullptr) {}

DeviceManager::~DeviceManager() {
  cdda_.reset();
  StopVolumeMonitor();
}

void DeviceManager::Init() {
  if (device_db_) {
    device_db_->Init();
  }
  StartVolumeMonitor();
  Rescan();
}

void DeviceManager::StartVolumeMonitor() {
#ifdef HAVE_GIO
  if (volume_monitor_) {
    return;
  }
  auto *monitor = g_volume_monitor_get();
  volume_monitor_ = monitor;
  auto added = +[](GVolumeMonitor *, GObject *, gpointer data) {
    static_cast<DeviceManager *>(data)->ScheduleRescan();
  };
  g_signal_connect(monitor, "volume-added", G_CALLBACK(added), this);
  g_signal_connect(monitor, "volume-removed", G_CALLBACK(added), this);
  g_signal_connect(monitor, "mount-added", G_CALLBACK(added), this);
  g_signal_connect(monitor, "mount-removed", G_CALLBACK(added), this);
#endif
}

void DeviceManager::StopVolumeMonitor() {
  if (rescan_idle_) {
    g_source_remove(rescan_idle_);
    rescan_idle_ = 0;
  }
#ifdef HAVE_GIO
  if (volume_monitor_) {
    g_signal_handlers_disconnect_by_data(volume_monitor_, this);
    g_object_unref(static_cast<GVolumeMonitor *>(volume_monitor_));
    volume_monitor_ = nullptr;
  }
#endif
}

void DeviceManager::ScheduleRescan() {
  if (rescan_idle_) {
    return;
  }
  rescan_idle_ = g_idle_add(
      +[](gpointer data) -> gboolean {
        auto *self = static_cast<DeviceManager *>(data);
        self->rescan_idle_ = 0;
        self->Rescan();
        return G_SOURCE_REMOVE;
      },
      this);
}

std::string DeviceManager::MtpSerial(const std::string &unique_id) {
  if (unique_id.rfind("mtp:", 0) == 0) {
    return unique_id.substr(4);
  }
  return unique_id;
}

const ConnectedDevice *DeviceManager::FindDevice(const std::string &device_id) const {
  for (const ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id || device.friendly_name == device_id) {
      return &device;
    }
  }
  return nullptr;
}

SongList DeviceManager::SongsFromDirectory(const std::string &path, const std::function<void(int, int)> &progress) {
  SongList songs;
  if (path.empty()) {
    return songs;
  }
  const std::vector<std::string> entries = FileUtils::ListDirectoryRecursive(path);
  std::vector<std::string> audio;
  for (const std::string &entry : entries) {
    if (Song::IsAudioFile(entry)) {
      audio.push_back(entry);
    }
  }
  int done = 0;
  const int total = static_cast<int>(audio.size());
  if (progress) {
    progress(0, total);
  }
  for (const std::string &entry : audio) {
    Song song(Song::Source::Device);
    song.set_url(FileUtils::UriFromPath(entry));
    song.set_basefilename(FileUtils::BaseName(entry));
    song.set_valid(true);
    TagLib::FileRef file(entry.c_str(), true, TagLib::AudioProperties::Fast);
    if (!file.isNull() && file.tag()) {
      const TagLib::Tag *tag = file.tag();
      song.set_title(tag->title().to8Bit(true));
      song.set_artist(tag->artist().to8Bit(true));
      song.set_album(tag->album().to8Bit(true));
      song.set_genre(tag->genre().to8Bit(true));
      song.set_year(static_cast<int>(tag->year()));
      song.set_track(static_cast<int>(tag->track()));
      song.set_comment(tag->comment().to8Bit(true));
    }
    if (file.audioProperties()) {
      song.set_length_nanosec(static_cast<int64_t>(file.audioProperties()->lengthInMilliseconds()) * 1000000LL);
      song.set_bitrate(file.audioProperties()->bitrate());
      song.set_samplerate(file.audioProperties()->sampleRate());
    }
    if (song.title().empty()) {
      song.set_title(FileUtils::BaseName(entry));
    }
    songs.push_back(song);
    ++done;
    if (progress) {
      progress(done, total);
    }
  }
  std::sort(songs.begin(), songs.end(), [](const Song &a, const Song &b) { return a.PrettyTitleWithArtist() < b.PrettyTitleWithArtist(); });
  return songs;
}

SongList DeviceManager::MakeCddaSongs(int first_track, int last_track, const std::vector<int64_t> &lengths_nanosec) {
  SongList songs;
  if (first_track <= 0 || last_track < first_track) {
    return songs;
  }
  for (int track = first_track; track <= last_track; ++track) {
    Song song(Song::Source::CDDA);
    song.set_url("cdda://" + std::to_string(track));
    song.set_title("Track " + std::to_string(track));
    song.set_track(track);
    song.set_filetype(Song::FileType::CDDA);
    const size_t index = static_cast<size_t>(track - first_track);
    if (index < lengths_nanosec.size()) {
      song.set_length_nanosec(lengths_nanosec[index]);
    }
    song.set_valid(true);
    songs.push_back(song);
  }
  return songs;
}

void DeviceManager::Rescan() {
  devices_.clear();
#ifdef HAVE_GIO
  const std::vector<ConnectedDevice> volumes = GioLister().List();
  devices_.insert(devices_.end(), volumes.begin(), volumes.end());
#endif
#ifdef HAVE_UDISKS2
  {
    const std::vector<ConnectedDevice> udisks = Udisks2Lister().List();
    devices_.insert(devices_.end(), udisks.begin(), udisks.end());
  }
#endif
  const std::vector<ConnectedDevice> cds = CddaLister().List();
  devices_.insert(devices_.end(), cds.begin(), cds.end());
#ifdef HAVE_MTP
  {
    MtpConnection::InitLibMtp();
    LIBMTP_raw_device_t *raw = nullptr;
    int count = 0;
    if (LIBMTP_Detect_Raw_Devices(&raw, &count) == LIBMTP_ERROR_NONE && raw) {
      for (int i = 0; i < count; ++i) {
        ConnectedDevice entry;
        entry.backend = "mtp";
        entry.icon = "multimedia-player-symbolic";
        LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
        if (device) {
          char *name = LIBMTP_Get_Friendlyname(device);
          char *serial = LIBMTP_Get_Serialnumber(device);
          entry.friendly_name = name && *name ? name : "MTP device";
          entry.unique_id = std::string("mtp:") + (serial ? serial : std::to_string(i));
          free(name);
          free(serial);
          LIBMTP_Release_Device(device);
        } else {
          entry.friendly_name = "MTP device";
          entry.unique_id = "mtp:" + std::to_string(i);
        }
        devices_.push_back(entry);
        if (device_db_) {
          DeviceDatabaseBackend::Device stored;
          stored.unique_id = entry.unique_id;
          stored.friendly_name = entry.friendly_name;
          stored.icon_name = entry.icon;
          device_db_->AddDevice(stored);
        }
      }
      free(raw);
    }
  }
#endif
#ifdef HAVE_GPOD
  for (ConnectedDevice &device : devices_) {
    if (device.mount_path.empty()) {
      continue;
    }
    if (FileUtils::Exists(FileUtils::Join(device.mount_path, "iPod_Control")) ||
        FileUtils::Exists(FileUtils::Join(device.mount_path, "iTunes_Control"))) {
      device.backend = "gpod";
      device.icon = "multimedia-player-symbolic";
      if (device.friendly_name.find("iPod") == std::string::npos) {
        device.friendly_name += " (iPod)";
      }
    }
  }
#endif
#ifdef HAVE_AUDIOCD
  if (CddaHelpers::ShouldAddGenericCdda(static_cast<int>(cds.size()))) {
    ConnectedDevice cd;
    cd.friendly_name = "Audio CD";
    cd.unique_id = "cdda";
    cd.icon = "media-optical-symbolic";
    cd.backend = "cdda";
    devices_.push_back(cd);
  }
#endif
  if (device_db_) {
    for (const DeviceDatabaseBackend::Device &stored : device_db_->GetAllDevices()) {
      if (FindDevice(stored.unique_id) || IsForgotten(stored.unique_id)) {
        continue;
      }
      ConnectedDevice remembered;
      remembered.unique_id = stored.unique_id;
      remembered.friendly_name = stored.friendly_name.empty() ? stored.unique_id : stored.friendly_name;
      remembered.icon = stored.icon_name;
      remembered.backend = "filesystem";
      remembered.remembered = true;
      devices_.push_back(remembered);
    }
  }
  devices_.erase(std::remove_if(devices_.begin(), devices_.end(),
                                [this](const ConnectedDevice &device) { return IsForgotten(device.unique_id); }),
                 devices_.end());
  for (ConnectedDevice &device : devices_) {
    const auto it = song_counts_.find(device.unique_id);
    if (it != song_counts_.end()) {
      device.song_count = it->second;
    }
  }
  EnsureCddaWatch();
  DevicesChanged.Emit();
}

void DeviceManager::EnsureCddaWatch() {
#ifdef HAVE_AUDIOCD
  bool have_cdda = false;
  for (const ConnectedDevice &device : devices_) {
    if (device.backend == "cdda") {
      have_cdda = true;
      break;
    }
  }
  if (!have_cdda) {
    cdda_.reset();
    return;
  }
  if (cdda_) {
    return;
  }
  ConnectedDevice cd;
  cd.backend = "cdda";
  cd.unique_id = "cdda";
  for (const ConnectedDevice &device : devices_) {
    if (device.backend == "cdda") {
      cd = device;
      break;
    }
  }
  cdda_ = std::make_unique<CddaDevice>(cd);
  cdda_->DiscChanged.Connect([this]() { OnCddaDiscChanged(); });
  if (!cdda_->Init()) {
    cdda_.reset();
  }
#else
  cdda_.reset();
#endif
}

void DeviceManager::OnCddaDiscChanged() {
  if (cdda_ && CddaDiscChange::ShouldPauseWatchWhileLoading()) {
    cdda_->WatchForDiscChanges(false);
    cdda_->set_loader_active(true);
  }
  const SongList songs = SongsFromCdda();
  if (cdda_) {
    if (CddaDiscChange::ShouldAckAfterLoad()) {
      cdda_->AckMediaChanged();
    }
    cdda_->set_loader_active(false);
    cdda_->WatchForDiscChanges(true);
  }
  for (ConnectedDevice &device : devices_) {
    if (device.backend == "cdda") {
      RememberSongCount(device.unique_id, static_cast<int>(songs.size()));
    }
  }
  DevicesChanged.Emit();
}

bool DeviceManager::IsForgotten(const std::string &device_id) const {
  return std::find(forgotten_.begin(), forgotten_.end(), device_id) != forgotten_.end();
}

SongList DeviceManager::Songs(const std::string &device_id) {
  const ConnectedDevice *device = FindDevice(device_id);
  if (!device) {
    return {};
  }
  scan_device_id_ = device_id;
  last_scan_percent_ = -1;
  if (task_manager_) {
    scan_task_id_ = task_manager_->StartTask(DeviceScanProgress::TaskName());
  }
  SetUpdatingPercent(device_id, 0);
  auto progress = [this, device_id](int done, int total) {
    const int percent = DeviceScanProgress::Percent(done, total);
    if (!DeviceScanProgress::ShouldReport(last_scan_percent_, percent)) {
      return;
    }
    last_scan_percent_ = percent;
    if (task_manager_ && scan_task_id_) {
      task_manager_->SetTaskProgress(scan_task_id_, done, total);
    }
    SetUpdatingPercent(device_id, percent);
  };
  SongList songs;
  if (device->backend == "cdda") {
    songs = SongsFromCdda();
  } else if (device->backend == "mtp") {
    songs = SongsFromMtp(*device);
  } else if (device->backend == "gpod") {
    songs = GPodLoader::LoadSongs(device->mount_path);
    if (songs.empty()) {
      std::string root = device->mount_path;
      const std::string music = FileUtils::Join(root, "iPod_Control/Music");
      if (FileUtils::IsDirectory(music)) {
        root = music;
      }
      songs = SongsFromDirectory(root, progress);
    }
  } else {
    songs = SongsFromDirectory(device->mount_path, progress);
  }
  RememberSongCount(device_id, static_cast<int>(songs.size()));
  SetUpdatingPercent(device_id, DeviceScanProgress::FinishedPercent());
  if (task_manager_ && scan_task_id_) {
    task_manager_->SetTaskFinished(scan_task_id_);
    scan_task_id_ = 0;
  }
  scan_device_id_.clear();
  return songs;
}

SongList DeviceManager::SongsFromCdda() const {
#ifdef HAVE_AUDIOCD
  CddaHelpers::EnsureInit();
  CdIo_t *cdio = cdio_open(nullptr, DRIVER_DEVICE);
  if (!cdio) {
    return {};
  }
  const track_t first = cdio_get_first_track_num(cdio);
  const track_t last = cdio_get_last_track_num(cdio);
  if (!CddaHelpers::IsValidTrackRange(first, last)) {
    cdio_destroy(cdio);
    return {};
  }
  std::vector<int64_t> lengths;
  for (track_t track = first; track <= last; ++track) {
    if (cdio_get_track_format(cdio, track) != TRACK_FORMAT_AUDIO) {
      lengths.push_back(0);
      continue;
    }
    const lsn_t start = cdio_get_track_lsn(cdio, track);
    const lsn_t end = cdio_get_track_last_lsn(cdio, track);
    const int64_t sectors = end >= start ? static_cast<int64_t>(end - start + 1) : 0;
    lengths.push_back(sectors * 1000000000LL / 75);
  }
  cdio_destroy(cdio);
  return MakeCddaSongs(first, last, lengths);
#else
  return {};
#endif
}

SongList DeviceManager::SongsFromMtp(const ConnectedDevice &device) const {
#ifdef HAVE_MTP
  SongList songs = MtpLoader::LoadSongs(MtpSerial(device.unique_id));
  if (device_db_) {
    const DeviceDatabaseBackend::Device stored = device_db_->FindByUniqueId(device.unique_id);
    if (!songs.empty() && stored.id >= 0) {
      device_db_->ReplaceSongs(stored.id, songs);
    } else if (songs.empty() && stored.id >= 0) {
      songs = device_db_->Songs(stored.id);
    }
  }
  return songs;
#else
  (void)device;
  return {};
#endif
}

std::string DeviceManager::DownloadMtpTrack(const std::string &url) const { return MtpDevice::DownloadTrack(url); }

UrlHandler::LoadResult DeviceManager::DeviceUrlHandler::Load(const std::string &url, AsyncCallback) {
  UrlHandler::LoadResult result;
  const std::string path = manager_->DownloadMtpTrack(url);
  if (path.empty()) {
    result.type = UrlHandler::LoadResult::Type::Error;
    result.error = DeviceError::MtpCopyFailed();
    if (manager_) {
      manager_->DeviceError.Emit(result.error);
    }
    return result;
  }
  result.type = UrlHandler::LoadResult::Type::TrackAvailable;
  result.media_url = url;
  result.stream_url = FileUtils::UriFromPath(path);
  return result;
}

std::string DeviceManager::MusicPath(const ConnectedDevice &device) {
  if (device.mount_path.empty()) {
    return {};
  }
  if (device.backend == "gpod") {
    return device.mount_path;
  }
  return FileUtils::Join(device.mount_path, "Music");
}

SongList DeviceManager::TranscodeForDevice(const SongList &songs, const ConnectedDevice &device) const {
  const DeviceDatabaseBackend::Device stored = StoredDevice(device.unique_id);
  const MusicStorage::TranscodeMode mode =
      OrganizeTranscode::FromDeviceMode(stored.id >= 0 ? stored.transcode_mode : DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported);
  const Song::FileType format = stored.id >= 0 ? stored.transcode_format : Song::FileType::MPEG;
  const std::vector<Song::FileType> supported = DeviceCopySupported::ForDevice(device);
  SongList prepared;
  prepared.reserve(songs.size());
  for (const Song &song : songs) {
    const Song::FileType dest_type = OrganizeTranscode::Check(song.filetype(), mode, format, supported);
    if (dest_type == Song::FileType::Unknown || !OrganizeTranscode::CanTranscode(dest_type)) {
      prepared.push_back(song);
      continue;
    }
    const std::string src = FileUtils::PathFromUri(song.url());
    const std::string temp = OrganizeTranscode::FiddleExtension(
        FileUtils::Join(StandardPaths::CacheDir(), "device-" + FileUtils::BaseName(src)), OrganizeTranscode::ExtensionForFileType(dest_type));
    Transcoder transcoder;
    if (!transcoder.TranscodeFile(song, temp, OrganizeTranscode::FormatFromFileType(dest_type))) {
      LogWarning("Could not transcode %s for device %s", src.c_str(), device.unique_id.c_str());
      continue;
    }
    Song copy = song;
    copy.set_url(FileUtils::UriFromPath(temp));
    copy.set_filetype(dest_type);
    copy.set_basefilename(FileUtils::BaseName(temp));
    prepared.push_back(copy);
  }
  return prepared;
}

bool DeviceManager::CopySongs(const std::string &device_id, const SongList &songs) {
  const ConnectedDevice *found = FindDevice(device_id);
  if (!found) {
    DeviceError.Emit(DeviceError::MissingDevice());
    return false;
  }
  const ConnectedDevice target = *found;
  const DeviceDatabaseBackend::Device stored = StoredDevice(target.unique_id);
  DeviceCopyRunner runner(task_manager_, tagreader_);
  runner.set_transcode(OrganizeTranscode::FromDeviceMode(stored.id >= 0 ? stored.transcode_mode
                                                                       : DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported),
                       stored.id >= 0 ? stored.transcode_format : Song::FileType::MPEG);
  if (!runner.Copy(target, songs)) {
    DeviceError.Emit(DeviceError::CopyFailed());
    return false;
  }
  RefreshAfterCopy(device_id, runner.copied(), runner.copied_songs());
  return true;
}

void DeviceManager::RefreshAfterCopy(const std::string &device_id, int copied, const SongList &on_device) {
  const ConnectedDevice *device = FindDevice(device_id);
  const std::string backend = device ? device->backend : std::string();
  if (!DeviceCopyRefresh::ShouldRefreshAfterCopy(backend, copied)) {
    return;
  }
  if (device_db_ && !on_device.empty()) {
    const DeviceDatabaseBackend::Device stored = device_db_->FindByUniqueId(device_id);
    if (stored.id >= 0) {
      SongList songs = device_db_->Songs(stored.id);
      songs.insert(songs.end(), on_device.begin(), on_device.end());
      device_db_->ReplaceSongs(stored.id, songs);
    }
  }
  if (song_counts_.count(device_id) && song_counts_[device_id] >= 0) {
    RememberSongCount(device_id, song_counts_[device_id] + copied);
  }
  DevicesChanged.Emit();
}

void DeviceManager::Remember(const std::string &device_id) {
  if (!device_db_) {
    return;
  }
  const ConnectedDevice *found = FindDevice(device_id);
  if (!found) {
    return;
  }
  if (device_db_->FindByUniqueId(device_id).id >= 0) {
    return;
  }
  DeviceDatabaseBackend::Device stored;
  stored.unique_id = found->unique_id;
  stored.friendly_name = found->friendly_name;
  stored.icon_name = found->icon;
  stored.size = found->size;
  device_db_->AddDevice(stored);
}

void DeviceManager::RememberSongCount(const std::string &device_id, const int count) {
  song_counts_[device_id] = count;
  for (ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id) {
      device.song_count = count;
      return;
    }
  }
}

void DeviceManager::SetUpdatingPercent(const std::string &device_id, const int percent) {
  for (ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id) {
      device.updating_percent = percent;
      DevicesChanged.Emit();
      return;
    }
  }
}

bool DeviceManager::Forget(const std::string &device_id) {
  if (device_id.empty()) {
    return false;
  }
  song_counts_.erase(device_id);
  if (!IsForgotten(device_id)) {
    forgotten_.push_back(device_id);
  }
  if (device_db_) {
    const DeviceDatabaseBackend::Device stored = device_db_->FindByUniqueId(device_id);
    if (stored.id >= 0) {
      device_db_->RemoveDevice(stored.id);
    }
  }
  Rescan();
  return true;
}

bool DeviceManager::Mount(const std::string &device_id) {
#ifdef HAVE_GIO
  if (device_id.empty()) {
    DeviceError.Emit(DeviceError::MountFailed());
    return false;
  }
  GVolumeMonitor *monitor = g_volume_monitor_get();
  if (!monitor) {
    return false;
  }
  GList *volumes = g_volume_monitor_get_volumes(monitor);
  GVolume *match = nullptr;
  for (GList *l = volumes; l; l = l->next) {
    GVolume *volume = G_VOLUME(l->data);
    gchar *id = g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    gchar *name = g_volume_get_name(volume);
    const bool same = (id && device_id == id) || (name && device_id == name);
    g_free(id);
    g_free(name);
    if (same) {
      match = volume;
      g_object_ref(match);
      break;
    }
  }
  for (GList *l = volumes; l; l = l->next) {
    g_object_unref(l->data);
  }
  g_list_free(volumes);
  g_object_unref(monitor);
  if (!match) {
    DeviceError.Emit(DeviceError::MountFailed());
    return false;
  }
  g_volume_mount(match, G_MOUNT_MOUNT_NONE, nullptr, nullptr,
                 +[](GObject *source, GAsyncResult *result, gpointer data) {
                   g_volume_mount_finish(G_VOLUME(source), result, nullptr);
                   static_cast<DeviceManager *>(data)->Rescan();
                   g_object_unref(source);
                 },
                 this);
  return true;
#else
  (void)device_id;
  DeviceError.Emit(DeviceError::MountFailed());
  return false;
#endif
}

bool DeviceManager::Unmount(const std::string &device_id) {
#ifdef HAVE_GIO
  const ConnectedDevice *found = FindDevice(device_id);
  if (!found || found->mount_path.empty()) {
    DeviceError.Emit(DeviceError::UnmountFailed());
    return false;
  }
  GFile *file = g_file_new_for_path(found->mount_path.c_str());
  if (!file) {
    return false;
  }
  g_file_unmount_mountable_with_operation(file, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    g_file_unmount_mountable_with_operation_finish(G_FILE(source), result, nullptr);
    static_cast<DeviceManager *>(data)->Rescan();
    g_object_unref(source);
  }, this);
  return true;
#else
  (void)device_id;
  DeviceError.Emit(DeviceError::UnmountFailed());
  return false;
#endif
}

bool DeviceManager::SetDeviceOptions(const std::string &device_id, const std::string &friendly_name,
                                     DeviceDatabaseBackend::TranscodeMode mode, Song::FileType format, const std::string &icon_name) {
  if (!device_db_) {
    return false;
  }
  DeviceDatabaseBackend::Device stored = device_db_->FindByUniqueId(device_id);
  const std::string icon = icon_name.empty() ? stored.icon_name : icon_name;
  if (stored.id < 0) {
    stored.unique_id = device_id;
    stored.friendly_name = friendly_name;
    stored.icon_name = icon;
    stored.transcode_mode = mode;
    stored.transcode_format = format;
    return device_db_->AddDevice(stored) >= 0;
  }
  device_db_->SetDeviceOptions(stored.id, friendly_name.empty() ? stored.friendly_name : friendly_name, icon, mode, format);
  for (ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id) {
      if (!friendly_name.empty()) {
        device.friendly_name = friendly_name;
      }
      if (!icon.empty()) {
        device.icon = icon;
      }
    }
  }
  return true;
}

DeviceDatabaseBackend::Device DeviceManager::StoredDevice(const std::string &device_id) const {
  if (!device_db_) {
    return {};
  }
  return device_db_->FindByUniqueId(device_id);
}

bool DeviceManager::DeleteSong(const std::string &device_id, const Song &song) {
  const ConnectedDevice *found = FindDevice(device_id);
  if (!found) {
    DeviceError.Emit(DeviceError::MissingDevice());
    return false;
  }
#ifdef HAVE_MTP
  if (found->backend == "mtp") {
    if (!MtpDevice::DeleteSong(MtpSerial(found->unique_id), song)) {
      DeviceError.Emit(DeviceError::DeleteFailed());
      return false;
    }
    return true;
  }
#endif
#ifdef HAVE_GPOD
  if (found->backend == "gpod") {
    if (!GPodDevice::DeleteSong(found->mount_path, song)) {
      DeviceError.Emit(DeviceError::DeleteFailed());
      return false;
    }
    return true;
  }
#endif
  if (!found->mount_path.empty()) {
    FilesystemMusicStorage storage(found->mount_path);
    MusicStorage::DeleteJob job;
    job.metadata = song;
    if (!storage.DeleteFromStorage(job)) {
      DeviceError.Emit(DeviceError::DeleteFailed());
      return false;
    }
    return true;
  }
  DeviceError.Emit(DeviceError::DeleteFailed());
  return false;
}
