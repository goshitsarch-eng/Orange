#ifndef STRAWBERRY_DEVICEDATABASEBACKEND_H
#define STRAWBERRY_DEVICEDATABASEBACKEND_H

#include "core/database.h"
#include "core/song.h"

#include <string>
#include <vector>

class DeviceDatabaseBackend {
 public:
  static const int kDeviceSchemaVersion;

  enum class TranscodeMode {
    Transcode_Never = 0,
    Transcode_Always = 1,
    Transcode_Unsupported = 2
  };

  struct Device {
    int id = -1;
    std::string unique_id;
    std::string friendly_name;
    int64_t size = 0;
    std::string icon_name;
    TranscodeMode transcode_mode = TranscodeMode::Transcode_Unsupported;
    Song::FileType transcode_format = Song::FileType::FLAC;
    int schema_version = kDeviceSchemaVersion;
  };

  explicit DeviceDatabaseBackend(Database *db);

  bool Init();
  std::vector<Device> GetAllDevices() const;
  int AddDevice(const Device &device);
  void RemoveDevice(int id);
  void SetDeviceOptions(int id, const std::string &friendly_name, const std::string &icon_name, TranscodeMode mode, Song::FileType format);
  Device FindByUniqueId(const std::string &unique_id) const;

  bool ReplaceSongs(int device_id, const SongList &songs);
  SongList Songs(int device_id) const;

 private:
  bool EnsureTables();
  std::string SongsTable(int device_id) const;

  Database *db_;
};

#endif
