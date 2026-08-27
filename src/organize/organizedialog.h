#ifndef STRAWBERRY_ORGANIZEDIALOG_H
#define STRAWBERRY_ORGANIZEDIALOG_H

#include "core/musicstorage.h"
#include "core/song.h"

#include <gtk/gtk.h>

#include <string>
#include <vector>

class Application;

class OrganizeDialog {
 public:
  struct Request {
    SongList songs;
    bool move = false;
    std::string destination;
    MusicStorage::TranscodeMode transcode_mode = MusicStorage::TranscodeMode::Transcode_Never;
    Song::FileType transcode_format = Song::FileType::Unknown;
    std::vector<Song::FileType> supported_filetypes;
    bool show_eject = false;
    std::string device_id;
    std::string playlist;
  };

  static void Show(GtkWindow *parent, Application *app, const SongList &songs = {}, bool move = false);
  static void Show(GtkWindow *parent, Application *app, const Request &request);
};

#endif
