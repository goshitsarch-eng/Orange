#ifndef STRAWBERRY_SONGLOADERINSERTER_H
#define STRAWBERRY_SONGLOADERINSERTER_H

#include "core/song.h"

#include <glib.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CollectionBackend;
class Playlist;
class SongLoader;
class TagReader;
class TaskManager;
class UrlHandlers;

class SongLoaderInserter {
 public:
  explicit SongLoaderInserter(TagReader *tagreader, TaskManager *task_manager = nullptr, UrlHandlers *url_handlers = nullptr,
                              CollectionBackend *collection_backend = nullptr);
  ~SongLoaderInserter();

  SongList Load(const std::vector<std::string> &urls) const;
  int Insert(Playlist *playlist, const std::vector<std::string> &urls, int row = -1) const;

  struct StartOptions {
    int row = -1;
    bool play_now = false;
    bool enqueue = false;
    bool enqueue_next = false;
    std::function<void()> finished;
    std::function<void(int)> play;
  };

  // Heap-allocated only: self-deletes after the metadata pass. Do not call from unit tests.
  void Start(Playlist *destination, const std::vector<std::string> &urls, const StartOptions &options);

 private:
  void InsertSongs();
  void AsyncLoad();
  void NotifyFinished();
  static gpointer AsyncThread(gpointer data);
  static gboolean PreloadIdle(gpointer data);
  static gboolean EffectiveIdle(gpointer data);

  TagReader *tagreader_ = nullptr;
  TaskManager *task_manager_ = nullptr;
  UrlHandlers *url_handlers_ = nullptr;
  CollectionBackend *collection_backend_ = nullptr;
  Playlist *destination_ = nullptr;
  int row_ = -1;
  bool play_now_ = false;
  bool enqueue_ = false;
  bool enqueue_next_ = false;
  std::function<void()> finished_;
  std::function<void(int)> play_;
  SongList songs_;
  SongList effective_songs_;
  std::string playlist_name_;
  std::vector<std::unique_ptr<SongLoader>> pending_;
};

#endif
