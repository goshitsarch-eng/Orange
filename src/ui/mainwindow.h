#ifndef STRAWBERRY_MAINWINDOW_H
#define STRAWBERRY_MAINWINDOW_H

#include "core/application.h"
#include "core/commandlineoptions.h"

#include <adwaita.h>
#include <gtk/gtk.h>

class MainWindow {
 public:
  MainWindow(AdwApplication *gtk_app, Application *app, const CommandlineOptions &options);
  ~MainWindow();

  GtkWindow *window() const { return GTK_WINDOW(window_); }
  void Present();
  void CommandlineReceived(const CommandlineOptions &options);

 private:
  void BuildUi();
  void BuildSidebar();
  void BuildPlaylist();
  void BuildPlayerBar();
  void ConnectSignals();
  void RefreshCollection(const std::string &filter = {});
  void RefreshPlaylist();
  void RefreshPlaylistsList();
  void RefreshRadio();
  void RefreshDevices();
  void UpdateNowPlaying();
  void UpdatePlaybackButtons();
  void OpenSettings();
  void OpenAbout();
  void AddFiles();
  void AddCollectionFolder();
  static void OnPlayPause(GtkButton *button, gpointer data);
  static void OnStop(GtkButton *button, gpointer data);
  static void OnNext(GtkButton *button, gpointer data);
  static void OnPrevious(GtkButton *button, gpointer data);
  static void OnLove(GtkButton *button, gpointer data);
  static void OnVolume(GtkRange *range, gpointer data);
  static void OnSeek(GtkRange *range, gpointer data);

  AdwApplication *gtk_app_;
  Application *app_;
  AdwApplicationWindow *window_ = nullptr;
  AdwToastOverlay *toast_overlay_ = nullptr;
  AdwViewStack *sidebar_stack_ = nullptr;
  GtkWidget *collection_list_ = nullptr;
  GtkWidget *playlist_list_ = nullptr;
  GtkWidget *playlists_list_ = nullptr;
  GtkWidget *queue_list_ = nullptr;
  GtkWidget *radio_list_ = nullptr;
  GtkWidget *files_list_ = nullptr;
  GtkWidget *devices_list_ = nullptr;
  GtkWidget *smart_list_ = nullptr;
  GtkWidget *streaming_list_ = nullptr;
  GtkWidget *lyrics_view_ = nullptr;
  GtkWidget *title_label_ = nullptr;
  GtkWidget *artist_label_ = nullptr;
  GtkWidget *cover_image_ = nullptr;
  GtkWidget *play_button_ = nullptr;
  GtkWidget *position_label_ = nullptr;
  GtkWidget *duration_label_ = nullptr;
  GtkWidget *seek_scale_ = nullptr;
  GtkWidget *volume_scale_ = nullptr;
  GtkWidget *status_label_ = nullptr;
  GtkWidget *analyzer_drawing_ = nullptr;
  GtkWidget *moodbar_drawing_ = nullptr;
  GtkWidget *waveform_drawing_ = nullptr;
  guint position_timeout_ = 0;
};

#endif
