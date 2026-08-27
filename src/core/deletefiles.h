#ifndef STRAWBERRY_DELETEFILES_H
#define STRAWBERRY_DELETEFILES_H

#include "core/musicstorage.h"
#include "core/signal.h"
#include "core/song.h"
#include "core/taskmanager.h"

#include <string>
#include <vector>

class DeleteFiles {
 public:
  DeleteFiles(TaskManager *task_manager, MusicStorage *storage, bool use_trash);
  ~DeleteFiles();

  void Start(const SongList &songs);
  void Start(const std::vector<std::string> &filenames);
  void Begin(const SongList &songs);
  void StartAsync(const SongList &songs);
  void ProcessSome();
  void Cancel();
  void IdleTick();

  bool finished() const { return finished_; }
  bool cancelled() const { return cancelled_; }
  int next_index() const { return next_; }
  int task_id() const { return task_id_; }
  const SongList &errors() const { return errors_; }

  Signal<SongList> Finished;

 private:
  void ProcessOne(const Song &song);
  void Complete();
  void ScheduleIdle();

  TaskManager *task_manager_ = nullptr;
  MusicStorage *storage_ = nullptr;
  bool use_trash_ = false;
  SongList songs_;
  SongList errors_;
  int next_ = 0;
  int task_id_ = 0;
  bool cancelled_ = false;
  bool finished_ = false;
  bool async_ = false;
  bool started_storage_ = false;
  unsigned idle_id_ = 0;
};

#endif
