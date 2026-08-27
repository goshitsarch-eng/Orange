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

  virtual TranscodeMode GetTranscodeMode() const { return TranscodeMode::Transcode_Never; }
  virtual Song::FileType GetTranscodeFormat() const { return Song::FileType::Unknown; }
  virtual bool GetSupportedFiletypes(std::vector<Song::FileType> *) { return true; }

  virtual bool StartCopy(std::vector<Song::FileType> *) { return true; }
  virtual bool CopyToStorage(const CopyJob &job, std::string &error_text) = 0;
  virtual bool FinishCopy(bool success, std::string &) { return success; }

  virtual void StartDelete() {}
  virtual bool DeleteFromStorage(const DeleteJob &job) = 0;
  virtual bool FinishDelete(bool success, std::string &) { return success; }

  virtual void Eject() {}
};

#endif
