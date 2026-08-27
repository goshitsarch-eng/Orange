#ifndef STRAWBERRY_DEVICECOPYRUNNER_H
#define STRAWBERRY_DEVICECOPYRUNNER_H

#include "core/musicstorage.h"
#include "core/signal.h"
#include "core/song.h"
#include "device/connecteddevice.h"

#include <memory>
#include <string>

class TagReader;
class TaskManager;
class MtpCopySession;
class GPodCopySession;

class DeviceCopyRunner {
 public:
  DeviceCopyRunner(TaskManager *task_manager, TagReader *tagreader);
  ~DeviceCopyRunner();

  void set_transcode(MusicStorage::TranscodeMode mode, Song::FileType format) {
    transcode_mode_ = mode;
    transcode_format_ = format;
  }
  void set_playlist(const std::string &name) { playlist_ = name; }

  bool Copy(const ConnectedDevice &device, const SongList &songs);
  void Begin(const ConnectedDevice &device, const SongList &songs);
  void StartAsync(const ConnectedDevice &device, const SongList &songs);
  void ProcessSome();
  void Cancel();
  void IdleTick();

  bool finished() const { return finished_; }
  bool cancelled() const { return cancelled_; }
  int next_index() const { return next_; }
  int copied() const { return copied_; }
  const SongList &errors() const { return errors_; }

  Signal<bool> Finished;

 private:
  bool OpenSession();
  bool CopyOnePrepared(const Song &song);
  Song PrepareSong(const Song &song);
  void Complete();
  void ScheduleIdle();

  TaskManager *task_manager_ = nullptr;
  TagReader *tagreader_ = nullptr;
  ConnectedDevice device_;
  SongList songs_;
  SongList errors_;
  int next_ = 0;
  int copied_ = 0;
  int task_id_ = 0;
  bool cancelled_ = false;
  bool finished_ = false;
  bool async_ = false;
  bool session_open_ = false;
  unsigned idle_id_ = 0;
  MusicStorage::TranscodeMode transcode_mode_ = MusicStorage::TranscodeMode::Transcode_Never;
  Song::FileType transcode_format_ = Song::FileType::Unknown;
  std::string playlist_;
  std::unique_ptr<MtpCopySession> mtp_;
  std::unique_ptr<GPodCopySession> gpod_;
};

#endif
