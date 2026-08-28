#include "device/mtpmusicstorage.h"

#include "utilities/fileutils.h"

MtpMusicStorage::MtpMusicStorage(std::string serial) : serial_(std::move(serial)) {}

bool MtpMusicStorage::StartCopy(std::vector<Song::FileType> *supported) {
  copied_songs_.clear();
  if (!session_.Open(serial_)) {
    return false;
  }
  if (supported && supported->empty()) {
    *supported = session_.SupportedFiletypes();
  }
  return true;
}

bool MtpMusicStorage::CopyToStorage(const CopyJob &job, std::string &error_text) {
  Song song = job.metadata;
  if (!job.source.empty()) {
    song.set_url(FileUtils::UriFromPath(job.source));
  }
  Song on_device;
  if (!session_.CopyOne(song, job.progress, &on_device)) {
    error_text = "MTP copy failed";
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

bool MtpMusicStorage::FinishCopy(bool success, std::string &) {
  session_.Close();
  return success;
}

bool MtpMusicStorage::DeleteFromStorage(const DeleteJob &job) { return MtpDevice::DeleteSong(serial_, job.metadata); }
