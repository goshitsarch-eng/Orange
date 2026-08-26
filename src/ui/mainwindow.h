#ifndef STRAWBERRY_MAINWINDOW_H
#define STRAWBERRY_MAINWINDOW_H

#include "collection/collectiongrouping.h"
#include "collection/collectionviewcontainer.h"
#include "context/contextview.h"
#include "core/application.h"
#include "core/commandlineoptions.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistcontainer.h"
#include "playlist/playlistlistcontainer.h"
#include "device/deviceviewcontainer.h"
#include "fileview/fileview.h"
#include "playlist/playlistsequence.h"
#include "queue/queueview.h"
#include "radios/radioviewcontainer.h"
#include "smartplaylists/smartplaylistsviewcontainer.h"
#include "streaming/streamingtabsview.h"
#include "widgets/multiloadingindicator.h"
#include "widgets/playingwidget.h"
#include "widgets/trackslider.h"
#include "widgets/volumeslider.h"

#include <adwaita.h>
#include <gtk/gtk.h>

#include <memory>
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
  void BuildUi();
  void BuildSidebar();
  void BuildPlaylist();
  void BuildPlayerBar();
  void BuildContext();
  void ConnectSignals();
  void RefreshCollection(const std::string &filter = {}, bool update_text = false);
  void RefreshPlaylist();
  void RefreshPlaylistsList();
  void RefreshQueue();
  void RefreshRadio();
  void RefreshDevices();
  void RefreshFiles();
  void RefreshStreaming();
  void SearchStreaming(const std::string &query);
  void SearchRadio(const std::string &query);
  void RefreshSmartPlaylists();
  void UpdateNowPlaying();
  void UpdatePlaybackButtons();
  void UpdateCover(const std::vector<unsigned char> &data);
  void OpenSettings();
  void OpenAbout();
  void AddFiles();
  void AddCollectionFolder();
  void AddCdTracks();
  void LoadPlaylistFile();
  void SavePlaylistFile();
  void NewPlaylist();
  void ClearPlaylist();
  void CloseCurrentPlaylist();
  void DeleteCurrentPlaylist();
  void RenameCurrentPlaylist();
  void UndoPlaylist();
  void RedoPlaylist();
  void CycleRepeat();
  void CycleShuffle();
  void CycleAnalyzer();
  void RunSmartPlaylist(const std::string &kind);
  void RefreshPlaylistTabs();
  void PlayRadioChannel(const RadioChannel &channel);
  void ShowPlaylistMenu(double x, double y);
  void SelectPlaylistRow(int index, bool add);
  std::vector<int> SelectedPlaylistRows() const;
  SongList SelectedSongs() const;
  void SortPlaylistBy(PlaylistColumn column);
  void RescanCollection(bool full);
  void StopAfterCurrent();
  void QueuePlayNext();
  void ShowInCollection();
  void OpenSelectedInFileManager();
  void CopySelectedToCollection(bool move);
  void AddSelectedToTranscoder();
  void RenumberTracks();
  void RemoveDuplicates();
  void RemoveUnavailable();
  void ShuffleCurrent();
  void RateSelected(int stars);
  void CopySelectedUrl();
  void ShowToast(const std::string &text);

  static void OnPlayPause(GtkButton *button, gpointer data);
  static void OnStop(GtkButton *button, gpointer data);
  static void OnNext(GtkButton *button, gpointer data);
  static void OnPrevious(GtkButton *button, gpointer data);
  static void OnLove(GtkButton *button, gpointer data);
  static void DrawAnalyzer(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
  static void DrawMoodbar(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
  static void DrawWaveform(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);

  AdwApplication *gtk_app_;
  Application *app_;
  AdwApplicationWindow *window_ = nullptr;
  AdwToastOverlay *toast_overlay_ = nullptr;
  AdwViewStack *sidebar_stack_ = nullptr;
  std::unique_ptr<CollectionViewContainer> collection_container_;
  std::unique_ptr<PlaylistContainer> playlist_container_;
  std::unique_ptr<PlaylistListContainer> playlist_list_container_;
  std::unique_ptr<ContextView> context_view_;
  std::unique_ptr<PlayingWidget> playing_widget_;
  std::unique_ptr<TrackSlider> track_slider_;
  std::unique_ptr<VolumeSlider> volume_slider_;
  std::unique_ptr<FileView> file_view_;
  std::unique_ptr<SmartPlaylistsViewContainer> smart_container_;
  std::unique_ptr<RadioViewContainer> radio_container_;
  std::unique_ptr<DeviceViewContainer> device_container_;
  std::unique_ptr<MultiLoadingIndicator> loading_indicator_;
  std::unique_ptr<QueueView> queue_view_;
  GtkWidget *queue_list_ = nullptr;
  GtkWidget *streaming_list_ = nullptr;
  GtkWidget *streaming_stack_ = nullptr;
  GtkWidget *streaming_service_drop_ = nullptr;
  std::vector<std::unique_ptr<StreamingTabsView>> streaming_views_;
  GtkWidget *play_button_ = nullptr;
  GtkWidget *status_label_ = nullptr;
  GtkWidget *analyzer_drawing_ = nullptr;
  GtkWidget *moodbar_drawing_ = nullptr;
  GtkWidget *waveform_drawing_ = nullptr;
  GtkWidget *repeat_button_ = nullptr;
  GtkWidget *shuffle_button_ = nullptr;
  std::string device_browse_id_;
  std::string streaming_service_name_;
  std::string radio_query_;
  CollectionGrouping::Grouping grouping_;
  PlaylistSequence playlist_sequence_;
  std::string playlist_filter_;
  std::string collection_text_filter_;
  PlaylistColumn sort_column_ = PlaylistColumn::Title;
  bool sort_descending_ = false;
  std::vector<int> selected_playlist_rows_;
  std::string selection_playlist_name_;
  guint position_timeout_ = 0;
};

#endif
