#ifndef STRAWBERRY_MAINWINDOW_H
#define STRAWBERRY_MAINWINDOW_H

#include "core/application.h"
#include "core/commandlineoptions.h"

#include <adwaita.h>
#include <gtk/gtk.h>

#include <string>
#include <vector>

class MainWindow {
 public:
  MainWindow(AdwApplication *gtk_app, Application *app, const CommandlineOptions &options);
  ~MainWindow();

  GtkWindow *window() const { return GTK_WINDOW(window_); }
  void Present();
  void CommandlineReceived(const CommandlineOptions &options);

 private:
  enum class PlaylistColumn {
    Track,
    Title,
    Artist,
    Album,
    AlbumArtist,
    Performer,
    Composer,
    Year,
    OriginalYear,
    Disc,
    Length,
    Genre,
    Samplerate,
    Bitdepth,
    Bitrate,
    URL,
    Filename,
    Filesize,
    Filetype,
    DateCreated,
    DateModified,
    PlayCount,
    SkipCount,
    LastPlayed,
    Comment,
    Grouping,
    Source,
    Moodbar,
    Rating,
    HasCUE,
    EBUR128I,
    EBUR128LRA,
    BPM,
    Mood,
    InitialKey,
    Count
  };

  void BuildUi();
  void BuildSidebar();
  void BuildPlaylist();
  void BuildPlayerBar();
  void BuildContext();
  void ConnectSignals();
  void RefreshCollection(const std::string &filter = {});
  void RefreshPlaylist();
  void RefreshPlaylistsList();
  void RefreshQueue();
  void RefreshRadio();
  void RefreshDevices();
  void RefreshFiles();
  void RefreshStreaming();
  void RefreshSmartPlaylists();
  void UpdateNowPlaying();
  void UpdatePlaybackButtons();
  void UpdateCover(const std::vector<unsigned char> &data);
  void OpenSettings();
  void OpenAbout();
  void AddFiles();
  void AddCollectionFolder();
  void LoadPlaylistFile();
  void SavePlaylistFile();
  void NewPlaylist();
  void ClearPlaylist();
  void UndoPlaylist();
  void RedoPlaylist();
  void CycleRepeat();
  void CycleShuffle();
  void CycleAnalyzer();
  void RunSmartPlaylist(const std::string &kind);
  void RefreshPlaylistTabs();
  std::string CollectionHeader(const Song &song) const;
  void PlayRadioChannel(const RadioChannel &channel);
  void ShowPlaylistMenu(double x, double y);
  std::vector<int> SelectedPlaylistRows() const;
  static std::string ColumnTitle(PlaylistColumn column);
  static std::string ColumnText(const Song &song, PlaylistColumn column);
  static int ColumnWidth(PlaylistColumn column);
  bool ColumnVisible(PlaylistColumn column) const;
  void SortPlaylistBy(PlaylistColumn column);
  void SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data, int pixel_size);

  static void OnPlayPause(GtkButton *button, gpointer data);
  static void OnStop(GtkButton *button, gpointer data);
  static void OnNext(GtkButton *button, gpointer data);
  static void OnPrevious(GtkButton *button, gpointer data);
  static void OnLove(GtkButton *button, gpointer data);
  static void OnVolume(GtkRange *range, gpointer data);
  static void OnSeek(GtkRange *range, gpointer data);
  static void DrawAnalyzer(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
  static void DrawMoodbar(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
  static void DrawWaveform(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);

  AdwApplication *gtk_app_;
  Application *app_;
  AdwApplicationWindow *window_ = nullptr;
  AdwToastOverlay *toast_overlay_ = nullptr;
  AdwViewStack *sidebar_stack_ = nullptr;
  GtkWidget *collection_list_ = nullptr;
  GtkWidget *playlist_grid_ = nullptr;
  GtkWidget *playlist_scroll_ = nullptr;
  GtkWidget *playlists_list_ = nullptr;
  GtkWidget *queue_list_ = nullptr;
  GtkWidget *radio_list_ = nullptr;
  GtkWidget *files_list_ = nullptr;
  GtkWidget *devices_list_ = nullptr;
  GtkWidget *smart_list_ = nullptr;
  GtkWidget *streaming_list_ = nullptr;
  GtkWidget *lyrics_view_ = nullptr;
  GtkWidget *context_title_ = nullptr;
  GtkWidget *context_artist_ = nullptr;
  GtkWidget *context_album_ = nullptr;
  GtkWidget *context_cover_ = nullptr;
  GtkWidget *context_meta_ = nullptr;
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
  GtkWidget *repeat_button_ = nullptr;
  GtkWidget *shuffle_button_ = nullptr;
  GtkWidget *playlist_summary_ = nullptr;
  GtkWidget *playlist_tabs_ = nullptr;
  std::string files_path_;
  std::string collection_group_ = "artist-album";
  std::string playlist_filter_;
  PlaylistColumn sort_column_ = PlaylistColumn::Title;
  bool sort_descending_ = false;
  guint position_timeout_ = 0;
};

#endif
