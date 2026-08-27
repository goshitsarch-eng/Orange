#ifndef COVERERRORMESSAGE_H
#define COVERERRORMESSAGE_H

#include <string>

namespace CoverErrorMessage {

// Qt AlbumCoverChoiceController / AlbumCoverManager Error strings.
inline std::string FailedToOpenForReading(const std::string &path, const std::string &detail) {
  std::string text = "Failed to open cover file " + path + " for reading";
  if (!detail.empty()) {
    text += ": " + detail;
  }
  return text + ".";
}

inline std::string CoverFileEmpty(const std::string &path) { return "Cover file " + path + " is empty."; }

inline std::string FailedToOpenForWriting(const std::string &path, const std::string &detail) {
  std::string text = "Failed to open cover file " + path + " for writing";
  if (!detail.empty()) {
    text += ": " + detail;
  }
  return text + ".";
}

inline std::string FailedWritingCover(const std::string &path, const std::string &detail = {}) {
  if (detail.empty()) {
    return "Failed writing cover to file " + path + ".";
  }
  return "Failed writing cover to file " + path + ": " + detail;
}

inline std::string FailedToDeleteCover(const std::string &path, const std::string &detail) {
  std::string text = "Failed to delete cover file " + path;
  if (!detail.empty()) {
    text += ": " + detail;
  }
  return text + ".";
}

inline std::string CouldNotSaveCover(const std::string &path) { return "Could not save cover to file " + path + "."; }

inline bool ShouldEmit(const std::string &message) { return !message.empty(); }

}  // namespace CoverErrorMessage

#endif  // COVERERRORMESSAGE_H
