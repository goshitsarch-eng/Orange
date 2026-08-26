#include "core/filesystemmusicstorage.h"

#include "utilities/fileutils.h"

FilesystemMusicStorage::FilesystemMusicStorage(std::string root) : root_(std::move(root)) {}

bool FilesystemMusicStorage::CopyToStorage(const CopyJob &job, std::string &error_text) {
  const std::string dest = job.destination.empty() ? FileUtils::Join(root_, FileUtils::BaseName(job.source)) : job.destination;
  if (job.source.empty() || !FileUtils::IsFile(job.source)) {
    error_text = "Missing source: " + job.source;
    return false;
  }
  if (!job.overwrite && FileUtils::Exists(dest)) {
    error_text = "Destination exists: " + dest;
    return false;
  }
  if (!FileUtils::CopyFile(job.source, dest)) {
    error_text = "Could not copy " + job.source;
    return false;
  }
  if (job.remove_original) {
    FileUtils::Remove(job.source);
  }
  if (job.progress) {
    job.progress(1.0f);
  }
  return true;
}

bool FilesystemMusicStorage::DeleteFromStorage(const DeleteJob &job) {
  const std::string path = FileUtils::PathFromUri(job.metadata.url());
  if (path.empty() || !FileUtils::Exists(path)) {
    return false;
  }
  return FileUtils::Remove(path);
}
