#ifndef STRAWBERRY_DEVICEMANAGER_H
#define STRAWBERRY_DEVICEMANAGER_H

#include "core/signal.h"
#include "core/song.h"
#include "core/urlhandlers.h"
#include "device/connecteddevice.h"
#include "device/devicedatabasebackend.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Database;
class TagReader;
class TaskManager;

class DeviceManager {
 public:
  explicit DeviceManager(Database *database = nullptr, TaskManager *task_manager = nullptr);
  ~DeviceManager();

  void Init();
  void set_tagreader(class TagReader *tagreader) { tagreader_ = tagreader; }
  void Rescan();
  const std::vector<ConnectedDevice> &devices() const { return devices_; }
  bool CopySongs(const std::string &device_id, const SongList &songs);
  bool DeleteSong(const std::string &device_id, const Song &song);
  bool Forget(const std::string &device_id);
  bool Mount(const std::string &device_id);
  bool Unmount(const std::string &device_id);
  void Remember(const std::string &device_id);
  void RememberSongCount(const std::string &device_id, int count);
  void SetUpdatingPercent(const std::string &device_id, int percent);
  bool SetDeviceOptions(const std::string &device_id, const std::string &friendly_name, DeviceDatabaseBackend::TranscodeMode mode,
                        Song::FileType format, const std::string &icon_name = {});
  DeviceDatabaseBackend::Device StoredDevice(const std::string &device_id) const;
  SongList Songs(const std::string &device_id);
  UrlHandler *url_handler() const { return url_handler_.get(); }
  DeviceDatabaseBackend *device_database() const { return device_db_.get(); }

  static SongList SongsFromDirectory(const std::string &path, const std::function<void(int, int)> &progress = {});
  static SongList MakeCddaSongs(int first_track, int last_track, const std::vector<int64_t> &lengths_nanosec);
  static std::string MusicPath(const ConnectedDevice &device);
  SongList TranscodeForDevice(const SongList &songs, const ConnectedDevice &device) const;

  Signal<> DevicesChanged;

 private:
  class DeviceUrlHandler : public UrlHandler {
   public:
    explicit DeviceUrlHandler(DeviceManager *manager) : manager_(manager) {}
    std::string scheme() const override { return "mtp"; }
    LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;

   private:
    DeviceManager *manager_;
  };

  const ConnectedDevice *FindDevice(const std::string &device_id) const;
  SongList SongsFromMtp(const ConnectedDevice &device) const;
  SongList SongsFromCdda() const;
  std::string DownloadMtpTrack(const std::string &url) const;
  static std::string MtpSerial(const std::string &unique_id);

  void StartVolumeMonitor();
  void StopVolumeMonitor();
  void ScheduleRescan();

  bool IsForgotten(const std::string &device_id) const;

  std::vector<ConnectedDevice> devices_;
  std::vector<std::string> forgotten_;
  std::map<std::string, int> song_counts_;
  TaskManager *task_manager_ = nullptr;
  TagReader *tagreader_ = nullptr;
  int scan_task_id_ = 0;
  int last_scan_percent_ = -1;
  std::string scan_device_id_;
  std::unique_ptr<DeviceUrlHandler> url_handler_;
  std::unique_ptr<DeviceDatabaseBackend> device_db_;
  void *volume_monitor_ = nullptr;
  unsigned rescan_idle_ = 0;
};

#endif
