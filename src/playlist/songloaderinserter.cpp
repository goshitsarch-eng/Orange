#include "config.h"
#include "playlist/songloaderinserter.h"

#include "core/songloader.h"
#include "core/taskmanager.h"
#include "device/cddasongloader.h"
#include "playlist/playlist.h"
#include "playlist/playlistcdda.h"
#include "playlist/songloaderinserterplan.h"

#include <glib.h>

SongLoaderInserter::SongLoaderInserter(TagReader *tagreader, TaskManager *task_manager, UrlHandlers *url_handlers,
                                       CollectionBackend *collection_backend, NetworkAccessManager *network)
    : tagreader_(tagreader),
      task_manager_(task_manager),
      url_handlers_(url_handlers),
      collection_backend_(collection_backend),
      network_(network) {}

SongLoaderInserter::~SongLoaderInserter() = default;

SongList SongLoaderInserter::Load(const std::vector<std::string> &urls) const {
  if (!tagreader_) {
    return {};
  }
  SongLoader loader(url_handlers_, collection_backend_, tagreader_);
  const SongLoader::Result result = loader.LoadMany(urls);
  if (result == SongLoader::Result::BlockingLoadRequired) {
    loader.LoadFilenamesBlocking();
  }
  loader.LoadMetadataBlocking();
  return loader.songs();
}

int SongLoaderInserter::Insert(Playlist *playlist, const std::vector<std::string> &urls, int row) const {
  const SongList songs = Load(urls);
  if (!playlist || songs.empty()) {
    return 0;
  }
  if (row < 0) {
    playlist->AppendSongs(songs);
  } else {
    playlist->InsertSongs(row, songs);
  }
  return static_cast<int>(songs.size());
}

void SongLoaderInserter::Start(Playlist *destination, const std::vector<std::string> &urls, const StartOptions &options) {
  destination_ = destination;
  row_ = options.row;
  play_now_ = options.play_now;
  enqueue_ = options.enqueue;
  enqueue_next_ = options.enqueue_next;
  finished_ = options.finished;
  play_ = options.play;
  error_ = options.error;
  if (!tagreader_) {
    NotifyFinished();
    delete this;
    return;
  }
  for (const std::string &url : urls) {
    auto loader = std::make_unique<SongLoader>(url_handlers_, collection_backend_, tagreader_);
    const SongLoader::Result result = loader->Load(url);
    if (result == SongLoader::Result::BlockingLoadRequired) {
      pending_.push_back(std::move(loader));
      continue;
    }
    if (result == SongLoader::Result::Success) {
      songs_.insert(songs_.end(), loader->songs().begin(), loader->songs().end());
      if (!loader->playlist_name().empty()) {
        playlist_name_ = loader->playlist_name();
      }
    }
  }
  if (pending_.empty()) {
    InsertSongs();
    NotifyFinished();
    delete this;
    return;
  }
  g_thread_unref(g_thread_new("song-loader", SongLoaderInserter::AsyncThread, this));
}

void SongLoaderInserter::InsertSongs() {
  if (!destination_ || songs_.empty()) {
    return;
  }
  const int insert_at = SongLoaderInserterPlan::InsertAt(row_, destination_->row_count());
  if (row_ < 0) {
    destination_->AppendSongs(songs_);
  } else {
    destination_->InsertSongs(row_, songs_);
  }
  if (enqueue_) {
    for (size_t i = 0; i < songs_.size(); ++i) {
      destination_->queue()->Append(songs_[i], destination_->id(), insert_at + static_cast<int>(i));
    }
  } else if (enqueue_next_) {
    for (int i = static_cast<int>(songs_.size()) - 1; i >= 0; --i) {
      destination_->queue()->InsertNext(songs_[static_cast<size_t>(i)], destination_->id(), insert_at + i);
    }
  }
  if (play_now_ && play_) {
    play_(insert_at);
  }
}

void SongLoaderInserter::NotifyFinished() {
  if (finished_) {
    finished_();
  }
}

void SongLoaderInserter::EmitError(const std::string &error) {
  if (error_) {
    error_(PlaylistCdda::ErrorOrFallback(error));
  }
}

void SongLoaderInserter::DeleteLater() { g_idle_add(SongLoaderInserter::DeleteIdle, this); }

gboolean SongLoaderInserter::DeleteIdle(gpointer data) {
  delete static_cast<SongLoaderInserter *>(data);
  return G_SOURCE_REMOVE;
}

void SongLoaderInserter::LoadAudioCD(Playlist *destination, const StartOptions &options) {
  destination_ = destination;
  row_ = options.row;
  play_now_ = options.play_now;
  enqueue_ = options.enqueue;
  enqueue_next_ = options.enqueue_next;
  finished_ = options.finished;
  play_ = options.play;
  error_ = options.error;
#ifndef HAVE_AUDIOCD
  EmitError(PlaylistCdda::MissingPlaybackError());
  NotifyFinished();
  delete this;
  return;
#else
  cdda_ = std::make_unique<CddaSongLoader>();
  cdda_->SongsLoaded.Connect([this](const SongList &songs) {
    songs_ = songs;
    if (songs_.empty()) {
      EmitError(PlaylistCdda::EmptyError());
      return;
    }
    InsertSongs();
  });
  cdda_->SongsUpdated.Connect([this](const SongList &songs) {
    if (destination_) {
      destination_->UpdateItems(songs);
    }
  });
  cdda_->LoadError.Connect([this](const std::string &message) { EmitError(message); });
  cdda_->LoadingFinished.Connect([this]() {
    NotifyFinished();
    DeleteLater();
  });
  cdda_->Start(options.cdda_device, network_, options.cdda_fallbacks);
#endif
}

gpointer SongLoaderInserter::AsyncThread(gpointer data) {
  static_cast<SongLoaderInserter *>(data)->AsyncLoad();
  return nullptr;
}

void SongLoaderInserter::AsyncLoad() {
  int task_id = 0;
  if (task_manager_) {
    task_id = task_manager_->StartTask(SongLoaderInserterPlan::PreloadTaskName());
    task_manager_->SetTaskProgress(task_id, 0, static_cast<int>(pending_.size()));
  }
  bool first_loaded = false;
  int first_loaded_index = -1;
  for (size_t i = 0; i < pending_.size(); ++i) {
    SongLoader *loader = pending_[i].get();
    const SongLoader::Result result = loader->LoadFilenamesBlocking();
    if (task_manager_) {
      task_manager_->SetTaskProgress(task_id, static_cast<int>(i + 1), static_cast<int>(pending_.size()));
    }
    if (result == SongLoader::Result::Error) {
      continue;
    }
    if (SongLoaderInserterPlan::ShouldLoadFirstMetadata(first_loaded, true)) {
      loader->LoadMetadataBlocking();
      first_loaded = true;
      first_loaded_index = static_cast<int>(i);
    }
    songs_.insert(songs_.end(), loader->songs().begin(), loader->songs().end());
    if (!loader->playlist_name().empty()) {
      playlist_name_ = loader->playlist_name();
    }
  }
  if (task_manager_ && task_id > 0) {
    task_manager_->SetTaskFinished(task_id);
  }
  g_idle_add(SongLoaderInserter::PreloadIdle, this);

  task_id = 0;
  if (task_manager_) {
    task_id = task_manager_->StartTask(SongLoaderInserterPlan::MetadataTaskName());
    task_manager_->SetTaskProgress(task_id, 0, static_cast<int>(songs_.size()));
  }
  SongList songs;
  for (size_t i = 0; i < pending_.size(); ++i) {
    SongLoader *loader = pending_[i].get();
    if (static_cast<int>(i) != first_loaded_index) {
      loader->LoadMetadataBlocking();
    }
    songs.insert(songs.end(), loader->songs().begin(), loader->songs().end());
    if (task_manager_) {
      task_manager_->SetTaskProgress(task_id, static_cast<int>(songs.size()), static_cast<int>(songs_.size()));
    }
  }
  if (task_manager_ && task_id > 0) {
    task_manager_->SetTaskFinished(task_id);
  }
  effective_songs_ = songs;
  g_idle_add(SongLoaderInserter::EffectiveIdle, this);
}

gboolean SongLoaderInserter::PreloadIdle(gpointer data) {
  auto *self = static_cast<SongLoaderInserter *>(data);
  self->InsertSongs();
  self->NotifyFinished();
  return G_SOURCE_REMOVE;
}

gboolean SongLoaderInserter::EffectiveIdle(gpointer data) {
  auto *self = static_cast<SongLoaderInserter *>(data);
  if (self->destination_) {
    self->destination_->UpdateItems(self->effective_songs_);
  }
  self->NotifyFinished();
  delete self;
  return G_SOURCE_REMOVE;
}
