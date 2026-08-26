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

  void Start(const SongList &songs);
  void Start(const std::vector<std::string> &filenames);
  const SongList &errors() const { return errors_; }

  Signal<SongList> Finished;

 private:
  TaskManager *task_manager_ = nullptr;
  MusicStorage *storage_ = nullptr;
  bool use_trash_ = false;
  SongList errors_;
};

#endif
