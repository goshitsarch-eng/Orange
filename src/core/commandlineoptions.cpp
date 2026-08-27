#include "core/commandlineoptions.h"

#include "core/commandlinewindow.h"
#include "version.h"

#include <glib.h>

#include <cstring>

bool CommandlineOptions::Parse(int argc, char **argv) {
  gboolean play = false;
  gboolean play_pause = false;
  gboolean pause = false;
  gboolean stop = false;
  gboolean previous = false;
  gboolean next = false;
  gboolean restart_or_previous = false;
  gboolean stop_after_current = false;
  gboolean show_osd = false;
  gboolean toggle_pretty_osd = false;
  gboolean debug = false;
  gboolean version = false;
  gboolean quiet = false;
  gboolean verbose = false;
  gboolean append = false;
  gboolean load = false;
  gchar *create_new = nullptr;
  gchar *play_playlist = nullptr;
  gchar *language = nullptr;
  gchar *log_levels = nullptr;
  gchar *resize_window = nullptr;
  gint volume = -1;
  gint volume_increase = 0;
  gint volume_decrease = 0;
  gint seek_to = -1;
  gint seek_by = 0;
  gint play_track = -1;

  const GOptionEntry entries[] = {
      {"play", 0, 0, G_OPTION_ARG_NONE, &play, "Start playing", nullptr},
      {"play-pause", 0, 0, G_OPTION_ARG_NONE, &play_pause, "Play if stopped, pause if playing", nullptr},
      {"pause", 0, 0, G_OPTION_ARG_NONE, &pause, "Pause playback", nullptr},
      {"stop", 0, 0, G_OPTION_ARG_NONE, &stop, "Stop playback", nullptr},
      {"stop-after-current", 0, 0, G_OPTION_ARG_NONE, &stop_after_current, "Stop after the current track", nullptr},
      {"previous", 0, 0, G_OPTION_ARG_NONE, &previous, "Skip backward", nullptr},
      {"restart-or-previous", 0, 0, G_OPTION_ARG_NONE, &restart_or_previous, "Restart the current track or go to the previous track", nullptr},
      {"next", 0, 0, G_OPTION_ARG_NONE, &next, "Skip forward", nullptr},
      {"play-playlist", 0, 0, G_OPTION_ARG_STRING, &play_playlist, "Play the named playlist", "NAME"},
      {"append", 'a', 0, G_OPTION_ARG_NONE, &append, "Append files/URLs to the playlist", nullptr},
      {"load", 'l', 0, G_OPTION_ARG_NONE, &load, "Replace the current playlist with files/URLs", nullptr},
      {"create", 'c', 0, G_OPTION_ARG_STRING, &create_new, "Create a new playlist from files/URLs", "NAME"},
      {"volume", 0, 0, G_OPTION_ARG_INT, &volume, "Set the volume to LEVEL (0-100)", "LEVEL"},
      {"volume-increase", 0, 0, G_OPTION_ARG_INT, &volume_increase, "Increase volume by LEVEL", "LEVEL"},
      {"volume-decrease", 0, 0, G_OPTION_ARG_INT, &volume_decrease, "Decrease volume by LEVEL", "LEVEL"},
      {"seek-to", 0, 0, G_OPTION_ARG_INT, &seek_to, "Seek to POSITION seconds", "POSITION"},
      {"seek-by", 0, 0, G_OPTION_ARG_INT, &seek_by, "Seek by OFFSET seconds", "OFFSET"},
      {"play-track", 0, 0, G_OPTION_ARG_INT, &play_track, "Play the track at INDEX", "INDEX"},
      {"show-osd", 0, 0, G_OPTION_ARG_NONE, &show_osd, "Display the on-screen-display", nullptr},
      {"toggle-pretty-osd", 0, 0, G_OPTION_ARG_NONE, &toggle_pretty_osd, "Toggle the pretty OSD", nullptr},
      {"language", 0, 0, G_OPTION_ARG_STRING, &language, "Override the language", "LANG"},
      {"debug", 0, 0, G_OPTION_ARG_NONE, &debug, "Enable debug output", nullptr},
      {"version", 'v', 0, G_OPTION_ARG_NONE, &version, "Print version and exit", nullptr},
      {"quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "Equivalent to --log-levels *:1", nullptr},
      {"verbose", 0, 0, G_OPTION_ARG_NONE, &verbose, "Equivalent to --log-levels *:4", nullptr},
      {"log-levels", 0, 0, G_OPTION_ARG_STRING, &log_levels, "Comma-separated log levels (e.g. *:4)", "LEVELS"},
      {"resize-window", 0, 0, G_OPTION_ARG_STRING, &resize_window, "Resize the main window", "WxH"},
      {nullptr, 0, 0, G_OPTION_ARG_NONE, nullptr, nullptr, nullptr},
  };

  GError *error = nullptr;
  GOptionContext *context = g_option_context_new(" [URL/FILE...]");
  g_option_context_add_main_entries(context, entries, nullptr);
  g_option_context_set_summary(context, "Strawberry Music Player " STRAWBERRY_VERSION_DISPLAY " (GTK 4 / Adwaita)");
  const gboolean parsed = g_option_context_parse(context, &argc, &argv, &error);
  g_option_context_free(context);
  if (!parsed) {
    if (error) {
      g_printerr("%s\n", error->message);
      g_error_free(error);
    }
    return false;
  }

  if (play) player_action_ = PlayerAction::Play;
  if (play_pause) player_action_ = PlayerAction::PlayPause;
  if (pause) player_action_ = PlayerAction::Pause;
  if (stop) player_action_ = PlayerAction::Stop;
  if (previous) player_action_ = PlayerAction::Previous;
  if (next) player_action_ = PlayerAction::Next;
  if (restart_or_previous) player_action_ = PlayerAction::RestartOrPrevious;
  if (stop_after_current) player_action_ = PlayerAction::StopAfterCurrent;
  if (play_playlist) {
    player_action_ = PlayerAction::PlayPlaylist;
    playlist_name_ = play_playlist;
    g_free(play_playlist);
  }
  if (load) url_list_action_ = UrlListAction::Load;
  if (append) url_list_action_ = UrlListAction::Append;
  if (create_new) {
    url_list_action_ = UrlListAction::CreateNew;
    playlist_name_ = create_new;
    g_free(create_new);
  }
  set_volume_ = volume;
  volume_modifier_ = volume_increase - volume_decrease;
  seek_to_ = seek_to;
  seek_by_ = seek_by;
  play_track_at_ = play_track;
  show_osd_ = show_osd;
  toggle_pretty_osd_ = toggle_pretty_osd;
  debug_ = debug;
  version_ = version;
  if (quiet) {
    log_levels_ = "*:1";
  }
  if (verbose) {
    log_levels_ = "*:4";
    debug_ = true;
  }
  if (log_levels) {
    log_levels_ = log_levels;
    g_free(log_levels);
  }
  if (resize_window) {
    int width = 0;
    int height = 0;
    if (CommandlineWindow::ParseSize(resize_window, &width, &height)) {
      player_action_ = PlayerAction::ResizeWindow;
      resize_width_ = width;
      resize_height_ = height;
    }
    g_free(resize_window);
  }
  if (language) {
    language_ = language;
    g_free(language);
  }

  for (int i = 1; i < argc; ++i) {
    urls_.emplace_back(argv[i]);
  }
  return true;
}

bool CommandlineOptions::is_empty() const {
  return player_action_ == PlayerAction::None && urls_.empty() && set_volume_ < 0 && volume_modifier_ == 0 &&
         seek_to_ < 0 && seek_by_ == 0 && play_track_at_ < 0 && !show_osd_ && !toggle_pretty_osd_;
}

bool CommandlineOptions::contains_play_options() const {
  return player_action_ != PlayerAction::None || !urls_.empty();
}
