#ifndef STRAWBERRY_ORGANIZE_H
#define STRAWBERRY_ORGANIZE_H

#include "core/musicstorage.h"
#include "core/signal.h"
#include "core/song.h"
#include "organize/organizeformat.h"
#include "organize/organizepreview.h"

#include <memory>
#include <string>
#include <vector>

class TaskManager;

class Organize {
 public:
  struct Error {
    std::string song;
    std::string message;
  };

  struct Options {
    bool move = false;
    bool overwrite = false;
    bool albumcover = false;
    MusicStorage::TranscodeMode transcode_mode = MusicStorage::TranscodeMode::Transcode_Never;
    Song::FileType transcode_format = Song::FileType::Unknown;
    std::vector<Song::FileType> supported_filetypes;
    class CollectionBackend *collection_backend = nullptr;
    class TagReader *tagreader = nullptr;
    int collection_directory_id = -1;
    bool destination_is_collection = false;
    std::string cover_cache_path;
    std::string playlist;
    MusicStorage *storage = nullptr;
    bool eject_after = false;
  };

  explicit Organize(TaskManager *task_manager = nullptr);
  ~Organize();

  std::vector<Error> Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move);
  std::vector<Error> Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, const Options &options);
  void Begin(const SongList &songs, const std::string &destination, const OrganizeFormat &format, const Options &options);
  void Start(const SongList &songs, const std::string &destination, const OrganizeFormat &format, const Options &options);
  void ProcessSome();
  void Cancel();
  void IdleTick();

  bool finished() const { return finished_; }
  bool cancelled() const { return cancelled_; }
  int next_index() const { return next_; }
  int task_id() const { return task_id_; }
  const std::vector<Error> &errors() const { return errors_; }
  MusicStorage *storage() const { return storage_; }

  static std::string CoverPathForSong(const Song &song);

  Signal<Organize *> Finished;
  Signal<int> FileCopied;

 private:
  void ProcessOne(const OrganizePreview::Entry &entry);
  void Complete();
  void ScheduleIdle();

  TaskManager *task_manager_ = nullptr;
  int task_id_ = 0;
  int next_ = 0;
  bool cancelled_ = false;
  bool finished_ = false;
  bool async_ = false;
  bool waiting_for_transcode_ = false;
  unsigned idle_id_ = 0;
  std::string destination_;
  OrganizeFormat format_;
  Options options_;
  std::vector<OrganizePreview::Entry> entries_;
  std::vector<Error> errors_;
  MusicStorage *storage_ = nullptr;
  std::unique_ptr<MusicStorage> owned_storage_;
};

#endif
