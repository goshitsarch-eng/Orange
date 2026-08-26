#include "device/devicemanager.h"

#include "config.h"
#include "core/logging.h"
#include "core/standardpaths.h"
#include "device/gpoddevice.h"
#include "device/gpodloader.h"
#include "utilities/fileutils.h"
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

#include <algorithm>
#include <functional>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>

DeviceManager::DeviceManager() : url_handler_(std::make_unique<DeviceUrlHandler>(this)) {}

DeviceManager::~DeviceManager() = default;

void DeviceManager::Init() { Rescan(); }

const ConnectedDevice *DeviceManager::FindDevice(const std::string &device_id) const {
  for (const ConnectedDevice &device : devices_) {
    if (device.unique_id == device_id || device.friendly_name == device_id) {
      return &device;
    }
  }
  return nullptr;
}

SongList DeviceManager::SongsFromDirectory(const std::string &path) {
  SongList songs;
  if (path.empty()) {
    return songs;
  }
  for (const std::string &entry : FileUtils::ListDirectoryRecursive(path)) {
    if (!Song::IsAudioFile(entry)) {
      continue;
    }
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
  GVolumeMonitor *monitor = g_volume_monitor_get();
  GList *volumes = g_volume_monitor_get_volumes(monitor);
  for (GList *l = volumes; l; l = l->next) {
    GVolume *volume = G_VOLUME(l->data);
    gchar *name = g_volume_get_name(volume);
    gchar *id = g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    ConnectedDevice device;
    device.friendly_name = name ? name : "Volume";
    device.unique_id = id ? id : device.friendly_name;
    device.icon = "drive-harddisk-usb-symbolic";
    device.backend = "gio";
    if (GMount *mount = g_volume_get_mount(volume)) {
      if (GFile *root = g_mount_get_root(mount)) {
        gchar *path = g_file_get_path(root);
        if (path) {
          device.mount_path = path;
          g_free(path);
        }
        g_object_unref(root);
      }
      g_object_unref(mount);
    }
    devices_.push_back(device);
    g_free(name);
    g_free(id);
    g_object_unref(volume);
  }
  g_list_free(volumes);
  g_object_unref(monitor);
#endif
#ifdef HAVE_MTP
  {
    LIBMTP_raw_device_t *raw = nullptr;
    int count = 0;
    LIBMTP_Init();
    if (LIBMTP_Detect_Raw_Devices(&raw, &count) == LIBMTP_ERROR_NONE && raw) {
      for (int i = 0; i < count; ++i) {
        LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
        ConnectedDevice entry;
        entry.backend = "mtp";
        entry.icon = "multimedia-player-symbolic";
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
  ConnectedDevice cd;
  cd.friendly_name = "Audio CD";
  cd.unique_id = "cdda";
  cd.icon = "media-optical-symbolic";
  cd.backend = "cdda";
  devices_.push_back(cd);
#endif
  DevicesChanged.Emit();
}

SongList DeviceManager::Songs(const std::string &device_id) const {
  const ConnectedDevice *device = FindDevice(device_id);
  if (!device) {
    return {};
  }
  if (device->backend == "cdda") {
    return SongsFromCdda();
  }
  if (device->backend == "mtp") {
    return SongsFromMtp(*device);
  }
  if (device->backend == "gpod") {
    SongList songs = GPodLoader::LoadSongs(device->mount_path);
    if (!songs.empty()) {
      return songs;
    }
    std::string root = device->mount_path;
    const std::string music = FileUtils::Join(root, "iPod_Control/Music");
    if (FileUtils::IsDirectory(music)) {
      root = music;
    }
    return SongsFromDirectory(root);
  }
  return SongsFromDirectory(device->mount_path);
}

SongList DeviceManager::SongsFromCdda() const {
#ifdef HAVE_AUDIOCD
  CdIo_t *cdio = cdio_open(nullptr, DRIVER_DEVICE);
  if (!cdio) {
    cdio = cdio_open(nullptr, DRIVER_UNKNOWN);
  }
  if (!cdio) {
    return {};
  }
  const track_t first = cdio_get_first_track_num(cdio);
  const track_t last = cdio_get_last_track_num(cdio);
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
  SongList songs;
#ifdef HAVE_MTP
  LIBMTP_raw_device_t *raw = nullptr;
  int count = 0;
  if (LIBMTP_Detect_Raw_Devices(&raw, &count) != LIBMTP_ERROR_NONE || !raw) {
    return songs;
  }
  LIBMTP_mtpdevice_t *mtp = nullptr;
  std::string serial;
  for (int i = 0; i < count; ++i) {
    LIBMTP_mtpdevice_t *opened = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
    if (!opened) {
      continue;
    }
    char *value = LIBMTP_Get_Serialnumber(opened);
    serial = value ? value : "";
    free(value);
    const std::string id = std::string("mtp:") + serial;
    if (id == device.unique_id || count == 1) {
      mtp = opened;
      break;
    }
    LIBMTP_Release_Device(opened);
  }
  free(raw);
  if (!mtp) {
    return songs;
  }
  LIBMTP_track_t *track = LIBMTP_Get_Tracklisting_With_Callback(mtp, nullptr, nullptr);
  if (track) {
    while (track) {
      LIBMTP_track_t *next = track->next;
      Song song(Song::Source::Device);
      song.set_url("mtp://" + serial + "/" + std::to_string(track->item_id));
      song.set_title(track->title && *track->title ? track->title : (track->filename ? track->filename : "Track"));
      song.set_artist(track->artist ? track->artist : "");
      song.set_album(track->album ? track->album : "");
      song.set_genre(track->genre ? track->genre : "");
      song.set_track(static_cast<int>(track->tracknumber));
      song.set_basefilename(track->filename ? track->filename : song.title());
      song.set_filesize(static_cast<int64_t>(track->filesize));
      if (track->duration > 0) {
        song.set_length_nanosec(static_cast<int64_t>(track->duration) * 1000000LL);
      }
      song.set_valid(true);
      songs.push_back(song);
      LIBMTP_destroy_track_t(track);
      track = next;
    }
  } else {
    LIBMTP_file_t *file = LIBMTP_Get_Filelisting_With_Callback(mtp, nullptr, nullptr);
    while (file) {
      LIBMTP_file_t *next = file->next;
      if (LIBMTP_FILETYPE_IS_AUDIO(file->filetype) && file->filename) {
        Song song(Song::Source::Device);
        song.set_url("mtp://" + serial + "/" + std::to_string(file->item_id));
        song.set_title(file->filename);
        song.set_basefilename(file->filename);
        song.set_filesize(static_cast<int64_t>(file->filesize));
        song.set_valid(true);
        songs.push_back(song);
      }
      LIBMTP_destroy_file_t(file);
      file = next;
    }
  }
  LIBMTP_Release_Device(mtp);
#else
  (void)device;
#endif
  return songs;
}

std::string DeviceManager::DownloadMtpTrack(const std::string &url) const {
#ifdef HAVE_MTP
  if (url.rfind("mtp://", 0) != 0) {
    return {};
  }
  const std::string rest = url.substr(6);
  const auto slash = rest.find('/');
  if (slash == std::string::npos) {
    return {};
  }
  const std::string serial = rest.substr(0, slash);
  const uint32_t item_id = static_cast<uint32_t>(std::strtoul(rest.substr(slash + 1).c_str(), nullptr, 10));
  const std::string dest = FileUtils::Join(StandardPaths::CacheDir(), "mtp-" + rest.substr(slash + 1));
  if (FileUtils::Exists(dest)) {
    return dest;
  }
  LIBMTP_raw_device_t *raw = nullptr;
  int count = 0;
  if (LIBMTP_Detect_Raw_Devices(&raw, &count) != LIBMTP_ERROR_NONE || !raw) {
    return {};
  }
  LIBMTP_mtpdevice_t *mtp = nullptr;
  for (int i = 0; i < count; ++i) {
    LIBMTP_mtpdevice_t *opened = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
    if (!opened) {
      continue;
    }
    char *value = LIBMTP_Get_Serialnumber(opened);
    const std::string id = value ? value : "";
    free(value);
    if (id == serial || count == 1) {
      mtp = opened;
      break;
    }
    LIBMTP_Release_Device(opened);
  }
  free(raw);
  if (!mtp) {
    return {};
  }
  const int ok = LIBMTP_Get_File_To_File(mtp, item_id, dest.c_str(), nullptr, nullptr);
  LIBMTP_Release_Device(mtp);
  return ok == 0 ? dest : std::string();
#else
  (void)url;
  return {};
#endif
}

UrlHandler::LoadResult DeviceManager::DeviceUrlHandler::Load(const std::string &url, AsyncCallback) {
  UrlHandler::LoadResult result;
  const std::string path = manager_->DownloadMtpTrack(url);
  if (path.empty()) {
    result.type = UrlHandler::LoadResult::Type::Error;
    result.error = "Could not copy the MTP track";
    return result;
  }
  result.type = UrlHandler::LoadResult::Type::TrackAvailable;
  result.media_url = url;
  result.stream_url = FileUtils::UriFromPath(path);
  return result;
}

bool DeviceManager::CopySongs(const std::string &device_id, const SongList &songs) {
  const ConnectedDevice *found = FindDevice(device_id);
  if (!found) {
    return false;
  }
  const ConnectedDevice target = *found;
#ifdef HAVE_MTP
  if (target.backend == "mtp") {
    LIBMTP_raw_device_t *raw = nullptr;
    int count = 0;
    if (LIBMTP_Detect_Raw_Devices(&raw, &count) != LIBMTP_ERROR_NONE || !raw) {
      return false;
    }
    LIBMTP_mtpdevice_t *mtp = nullptr;
    for (int i = 0; i < count; ++i) {
      LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
      if (!device) {
        continue;
      }
      char *serial = LIBMTP_Get_Serialnumber(device);
      const std::string id = std::string("mtp:") + (serial ? serial : "");
      free(serial);
      if (id == target.unique_id || count == 1) {
        mtp = device;
        break;
      }
      LIBMTP_Release_Device(device);
    }
    free(raw);
    if (!mtp) {
      return false;
    }
    int copied = 0;
    for (const Song &song : songs) {
      const std::string src = FileUtils::PathFromUri(song.url());
      if (src.empty() || !FileUtils::Exists(src)) {
        continue;
      }
      LIBMTP_file_t *file = LIBMTP_new_file_t();
      file->filename = strdup(FileUtils::BaseName(src).c_str());
      file->filesize = static_cast<uint64_t>(song.filesize() > 0 ? song.filesize() : 0);
      file->parent_id = 0;
      file->storage_id = 0;
      if (LIBMTP_Send_File_From_File(mtp, src.c_str(), file, nullptr, nullptr) == 0) {
        ++copied;
      }
      LIBMTP_destroy_file_t(file);
    }
    LIBMTP_Release_Device(mtp);
    LogInfo("Copied %d songs to MTP device", copied);
    return copied > 0;
  }
#endif
  if (target.mount_path.empty()) {
    LogInfo("Device %s has no mount path", device_id.c_str());
    return false;
  }
#ifdef HAVE_GPOD
  if (target.backend == "gpod") {
    return GPodDevice::CopySongs(target.mount_path, songs);
  }
#endif
#ifdef HAVE_GIO
  const std::string music = FileUtils::Join(target.mount_path, "Music");
  g_mkdir_with_parents(music.c_str(), 0755);
  int copied = 0;
  for (const Song &song : songs) {
    const std::string src = FileUtils::PathFromUri(song.url());
    if (src.empty() || !FileUtils::Exists(src)) {
      continue;
    }
    const std::string dest = FileUtils::Join(music, FileUtils::BaseName(src));
    if (FileUtils::CopyFile(src, dest)) {
      ++copied;
    }
  }
  LogInfo("Copied %d songs to %s", copied, music.c_str());
  return copied > 0;
#else
  (void)songs;
  return false;
#endif
}
