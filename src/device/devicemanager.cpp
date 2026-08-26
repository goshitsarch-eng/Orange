#include "device/devicemanager.h"

#include "config.h"
#include "core/logging.h"
#include "core/standardpaths.h"
#include "core/database.h"
#include "device/devicedatabasebackend.h"
#include "device/cddadevice.h"
#include "device/cddasongloader.h"
#include "device/filesystemdevice.h"
#include "device/giolister.h"
#include "device/gpoddevice.h"
#include "device/gpodloader.h"
#include "device/mtpconnection.h"
#include "device/mtpdevice.h"
#include "device/mtploader.h"
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

DeviceManager::DeviceManager(Database *database)
    : url_handler_(std::make_unique<DeviceUrlHandler>(this)),
      device_db_(database ? std::make_unique<DeviceDatabaseBackend>(database) : nullptr) {}

DeviceManager::~DeviceManager() = default;

void DeviceManager::Init() {
  if (device_db_) {
    device_db_->Init();
  }
  Rescan();
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
  const std::vector<ConnectedDevice> volumes = GioLister().List();
  devices_.insert(devices_.end(), volumes.begin(), volumes.end());
#endif
  {
    CddaDevice cd;
    const SongList tracks = cd.Songs();
    if (!tracks.empty()) {
      ConnectedDevice entry = cd.info();
      entry.unique_id = "cdda:default";
      devices_.push_back(entry);
    }
  }
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
    return MtpDevice::CopySongs(MtpSerial(target.unique_id), songs);
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

bool DeviceManager::DeleteSong(const std::string &device_id, const Song &song) {
  const ConnectedDevice *found = FindDevice(device_id);
  if (!found) {
    return false;
  }
#ifdef HAVE_MTP
  if (found->backend == "mtp") {
    return MtpDevice::DeleteSong(MtpSerial(found->unique_id), song);
  }
#endif
  (void)song;
  return false;
}
