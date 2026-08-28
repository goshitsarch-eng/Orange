#include "device/gpodmusicstorage.h"

#include "device/devicecopysupported.h"
#include "utilities/fileutils.h"

GPodMusicStorage::GPodMusicStorage(std::string mount_path) : mount_path_(std::move(mount_path)) {}

bool GPodMusicStorage::StartCopy(std::vector<Song::FileType> *supported) {
  copied_songs_.clear();
  opened_ = session_.Open(mount_path_);
  if (!opened_) {
    return false;
  }
  if (supported && supported->empty()) {
    *supported = DeviceCopySupported::ForCopy("gpod", {});
  }
  return true;
}

bool GPodMusicStorage::CopyToStorage(const CopyJob &job, std::string &error_text) {
  Song song = job.metadata;
  if (!job.source.empty()) {
    song.set_url(FileUtils::UriFromPath(job.source));
  }
  Song on_device;
  const std::string cover = job.albumcover ? job.cover_source : std::string();
  if (!session_.CopyOne(song, job.playlist, cover, &on_device)) {
    error_text = "iPod copy failed";
    return false;
  }
  if (on_device.is_valid()) {
    copied_songs_.push_back(on_device);
  }
  if (job.remove_original && !job.source.empty()) {
    FileUtils::Remove(job.source);
  }
  return true;
}

bool GPodMusicStorage::FinishCopy(bool success, std::string &error_text) {
  if (!opened_) {
    return success;
  }
  opened_ = false;
  if (!session_.Finish()) {
    error_text = "Writing iPod database failed";
    return false;
  }
  return success;
}

bool GPodMusicStorage::DeleteFromStorage(const DeleteJob &job) { return GPodDevice::DeleteSong(mount_path_, job.metadata); }
