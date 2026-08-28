#ifndef STRAWBERRY_MUSICSTORAGE_H
#define STRAWBERRY_MUSICSTORAGE_H

#include "core/song.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

class MusicStorage {
 public:
  enum class TranscodeMode { Transcode_Always = 1, Transcode_Never = 2, Transcode_Unsupported = 3 };

  using ProgressFunction = std::function<void(float)>;

  struct CopyJob {
    std::string source;
    std::string destination;
    Song metadata;
    bool overwrite = false;
    bool remove_original = false;
    bool albumcover = false;
    std::string cover_source;
    std::string cover_dest;
    ProgressFunction progress;
    std::string playlist;
  };

  struct DeleteJob {
    Song metadata;
    bool use_trash = false;
  };

  virtual ~MusicStorage() = default;

  virtual Song::Source source() const = 0;
  virtual std::string LocalPath() const { return {}; }
  virtual std::optional<int> collection_directory_id() const { return std::nullopt; }

  virtual TranscodeMode GetTranscodeMode() const { return transcode_mode_; }
  virtual Song::FileType GetTranscodeFormat() const { return transcode_format_; }
  virtual bool GetSupportedFiletypes(std::vector<Song::FileType> *) { return true; }

  void SetTranscodeMode(TranscodeMode mode) { transcode_mode_ = mode; }
  void SetTranscodeFormat(Song::FileType format) { transcode_format_ = format; }

  virtual bool StartCopy(std::vector<Song::FileType> *) { return true; }
  virtual bool CopyToStorage(const CopyJob &job, std::string &error_text) = 0;
  virtual bool FinishCopy(bool success, std::string &) { return success; }

  virtual void StartDelete() {}
  virtual bool DeleteFromStorage(const DeleteJob &job) = 0;
  virtual bool FinishDelete(bool success, std::string &) { return success; }

  // Qt ConnectedDevice::Eject calls DeviceManager::UnmountAsync when a mount path exists.
  void SetEjectHandler(std::function<void()> handler) { eject_handler_ = std::move(handler); }
  bool HasEjectHandler() const { return static_cast<bool>(eject_handler_); }
  virtual void Eject() {
    if (eject_handler_) {
      eject_handler_();
    }
  }

  // MTP/iPod adapters record on-device metadata for DeviceManager::RefreshAfterCopy.
  virtual SongList CopiedSongs() const { return {}; }

 private:
  TranscodeMode transcode_mode_ = TranscodeMode::Transcode_Never;
  Song::FileType transcode_format_ = Song::FileType::Unknown;
  std::function<void()> eject_handler_;
};

#endif
