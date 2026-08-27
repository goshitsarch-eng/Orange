#ifndef STRAWBERRY_MAINWINDOW_H
#define STRAWBERRY_MAINWINDOW_H

#include "collection/collectionbehaviour.h"
#include "collection/collectiongrouping.h"
#include "collection/collectionmodelupdate.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "desktop/taskbarprogress.h"
#include "collection/collectionviewcontainer.h"
#include "context/contextview.h"
#include "core/application.h"
#include "core/commandlineoptions.h"
#include "core/platforminterface.h"
#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "widgets/seekbarfade.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistcontainer.h"
#include "playlist/playlistlistcontainer.h"
#include "device/deviceviewcontainer.h"
#include "fileview/fileview.h"
#include "playlist/playlistsequence.h"
#include "queue/queueview.h"
#include "radios/radioviewcontainer.h"
#include "smartplaylists/smartplaylistsviewcontainer.h"
#include "streaming/streamingmetadataqueue.h"
#include "streaming/streamingtabsview.h"
#include "widgets/multiloadingindicator.h"
#include "widgets/playingwidget.h"
#include "widgets/trackslider.h"
#include "widgets/volumeslider.h"
#ifdef _WIN32
#include "core/windows7thumbbar.h"
#include "core/winsystemmediatransportcontrols.h"
#endif
#ifdef __APPLE__
#include "systemtrayicon/macsystemtrayicon.h"
#endif

#include <adwaita.h>
#include <gtk/gtk.h>

#include <memory>
#include <string>
#include <vector>

class FancyTabBar;
class QueuedErrorDialog;

class MainWindow : public PlatformInterface {
 public:
  MainWindow(AdwApplication *gtk_app, Application *app, const CommandlineOptions &options);
  ~MainWindow();

  GtkWindow *window() const { return GTK_WINDOW(window_); }
  void Present();
  void Activate() override;
  void CommandlineReceived(const CommandlineOptions &options);
  bool LoadUrl(const std::string &url) override;

 private:
  void HandlePlaylistsLoaded();
  void CheckFullRescanRevisions();
  void BuildUi();
  void BuildSidebar();
  void BuildPlaylist();
  void BuildPlayerBar();
  void BuildContext();
  void ConnectSignals();
  void RefreshCollection(const std::string &filter = {}, bool update_text = false);
  void ApplyCollectionIncremental(CollectionModelUpdateType type, const SongList &songs);
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
  void OpenSettings(const char *page_name = nullptr);
  void OpenAbout();
  void MaybeShowSponsor();
  void AddFiles();
  void AddPlaylistFolder();
  void AddCollectionFolder();
  void AddUrlsFromMenu(const std::vector<std::string> &urls);
  void AddCdTracks();
  void LoadPlaylistFile();
  void SavePlaylistFile();
  void SavePlaylistById(int id);
  void NewPlaylist();
  void ClearPlaylist();
  void RemoveSelectedPlaylistRows();
  void CloseCurrentPlaylist();
  void TryClosePlaylist(int id);
  void FinishClosePlaylist(int id);
  void HideToTray();
  void ToggleHide();
  void ToggleShowHide();
  void SelectPlayingTrack();
  void ApplyPlaylistBehaviour();
  void ApplyBackendSettings();
  void DeleteCurrentPlaylist();
  void RenameCurrentPlaylist();
  void RenamePlaylist(int id);
  void UndoPlaylist();
  void RedoPlaylist();
  void CycleRepeat();
  void CycleShuffle();
  void CycleAnalyzer();
  void ShowAnalyzerMenu();
  void ApplyAnalyzer();
  void EnsureAnalyzerTimer();
  void TickAnalyzer();
  void RunSmartPlaylist(const std::string &kind);
  void ActivateSmartPlaylist(const SmartPlaylistsItem &item);
  void RefreshPlaylistTabs();
  void GoToPlaylistIndex(int index);
  void NextPlaylistTab();
  void PreviousPlaylistTab();
  void LastPlaylistTab();
  void ActivePlaylistTab();
  void PlayRadioChannel(const RadioChannel &channel);
  void ShowPlaylistMenu(double x, double y);
  void PlaylistStopAfter();
  void PlayPlaylistMenuRow();
  void ShowCollectionMenu();
  void ShowStreamingMenu(const SongList &songs, StreamingCollectionActions::MenuContext ctx);
  StreamingTabsView *CurrentStreamingTabs() const;
  void StreamingFavorite(bool add);
  void StreamingAddToList(StreamingCollectionStore::List list);
  void StreamingSearchForThis();
  void ShowRadioMenu(const std::vector<RadioChannel> &channels);
  void ShowPlaylistListMenu(const std::string &name);
  void DropOnPlaylistList(const std::string &name, const std::string &payload, bool folder);
  void NewPlaylistFolder();
  void RenamePlaylistFolder(const std::string &path);
  void DeletePlaylistFolder(const std::string &path);
  void MovePlaylistToFolder(const std::string &name, const std::string &folder);
  void SelectPlaylistByName(const std::string &name);
  Playlist *PlaylistByName(const std::string &name) const;
  SongList SongsFromUrls(const std::vector<std::string> &urls) const;
  void ApplyCollectionPlan(const CollectionBehaviour::Plan &plan, const SongList &songs, const std::string &playlist_name = {});
  void ApplyCollectionPlanUrls(const CollectionBehaviour::Plan &plan, const std::vector<std::string> &paths,
                               const std::string &playlist_name = {});
  void ForceCompilationSelected(bool on);
  SongList CollectionSongs() const;
  bool EngineStopped() const;
  void ApplyBehaviourSettings();
  void ApplyAppearance();
  void RestoreGeometry();
  void SaveGeometry();
  void PlacePlayingWidget();
  void SelectPlaylistRow(int index, bool add);
  void RefreshPlaylistSummary();
  std::vector<int> SelectedPlaylistRows() const;
  SongList SelectedSongs() const;
  void SortPlaylistBy(PlaylistColumn column, PlaylistSortOrder order = PlaylistSortOrder::Toggle);
  void RescanCollection(bool full);
  void StopAfterCurrent();
  void QueuePlayNext();
  void ShowInCollection();
  void OpenSelectedInFileManager();
  void ShowInFileBrowser(const std::vector<std::string> &urls_or_paths);
  void ShowSongsInFileBrowser(const SongList &songs);
  SongList SongsFromFilePaths(const std::vector<std::string> &paths) const;
  void CopyFileViewToCollection(const std::vector<std::string> &paths, bool move);
  void CopySelectedToCollection(bool move);
  void AddSelectedToTranscoder();
  void RenumberTracks();
  void RemoveDuplicates();
  void RemoveUnavailable();
  void ShuffleCurrent();
  void RateSelected(int stars);
  void RateRow(int row, float rating);
  void RateRows(const std::vector<int> &rows, float rating);
  void ScrobbleCurrent();
  void UpdateScrobblerButtons();
  void CopySelectedUrl();
  void FocusSearchField();
  void SkipSelected();
  void JumpToPlaying();
  void RescanSelected();
  void FetchStreamingMetadata();
  void ProcessMetadataQueue();
  void ApplyFetchedMetadata(const StreamingMetadataQueue::Entry &entry, const Song &fetched, const std::string &error);
  void ScheduleMetadataQueue();
  void AddSelectedToPlaylist(int id);
  void AutoCompleteTags();
  void EditColumnValue();
  void SetColumnTo();
  void ApplyColumnValue(PlaylistColumn column, const std::string &value, const std::vector<int> &rows);
  void PersistEditedSongs(const std::vector<int> &rows);
  void FocusCollectionSearch();
  void FocusCollectionSearchFromKey(unsigned keyval);
  void ShowToast(const std::string &text);
  void ShowErrorDialog(const std::string &message);
  void CheckShowErrorDialog();
  void ApplySeekbarPlaybackState();
  void ApplySeekbarMode();
  void ApplySeekbarFadeWidgets();
  void StartSeekbarFadeTimer();
  void CycleSeekbarMode();
  void SetSeekbarMode(SeekbarSettings::Mode mode);
  static gboolean OnSeekbarFadeTick(gpointer data);
  void SetMoodbarStyle(MoodbarSettings::Style style);
  void ShowSeekbarMenu(GtkWidget *relative);
  void RememberHiddenWindowState();
  void RestoreAfterHide();
  void OnSeekbarScroll(double dy);
  void SeekFromBar(double x, int width);
  void SetShowSidebar(bool show);
  void ApplySidebar();
  void UpdatePlayingWidgetVisibility();
  void ApplyTabMode();
  void PersistTabSettings() const;
  void PopulateSidebarTabs();
  void ToggleMute();
  void ApplyMuteUi(unsigned volume);
  bool FocusIsEditable() const;
  gboolean OnWindowKeyCapture(guint keyval);
  gboolean OnWindowKeyBubble(guint keyval);

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
  std::unique_ptr<QueuedErrorDialog> error_dialog_;
  AdwToastOverlay *toast_overlay_ = nullptr;
  AdwViewStack *sidebar_stack_ = nullptr;
  GtkWidget *sidebar_box_ = nullptr;
  std::unique_ptr<FancyTabBar> sidebar_tabs_;
  GtkWidget *split_view_ = nullptr;
  GtkWidget *mute_button_ = nullptr;
  GSimpleAction *sidebar_action_ = nullptr;
  GSimpleAction *mute_action_ = nullptr;
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
  std::unique_ptr<AlbumCoverChoiceController> cover_controller_;
  GtkWidget *queue_list_ = nullptr;
  GtkWidget *streaming_list_ = nullptr;
  GtkWidget *streaming_stack_ = nullptr;
  GtkWidget *streaming_service_drop_ = nullptr;
  std::vector<std::unique_ptr<StreamingTabsView>> streaming_views_;
  GtkWidget *play_button_ = nullptr;
  GtkWidget *love_button_ = nullptr;
  GtkWidget *scrobble_button_ = nullptr;
  bool loved_current_track_ = false;
  std::string loved_song_url_;
  GtkWidget *status_bar_stack_ = nullptr;
  GtkWidget *status_label_ = nullptr;
  GtkWidget *analyzer_drawing_ = nullptr;
  GtkWidget *moodbar_drawing_ = nullptr;
  GtkWidget *waveform_drawing_ = nullptr;
  GtkWidget *seekbar_menu_ = nullptr;
  SeekbarFade::Machine moodbar_fade_;
  SeekbarFade::Machine waveform_fade_;
  guint seekbar_fade_source_ = 0;
  bool seekbar_fade_inited_ = false;
  int seekbar_wheel_accum_ = 0;
  GtkWidget *repeat_button_ = nullptr;
  GtkWidget *shuffle_button_ = nullptr;
  GtkWidget *collection_search_ = nullptr;
  std::string device_browse_id_;
  std::string streaming_service_name_;
  SongList streaming_menu_songs_;
  SongList radio_menu_songs_;
  std::vector<RadioChannel> radio_menu_channels_;
  std::string playlist_list_menu_name_;
  std::string playlist_list_menu_folder_;
  std::string radio_query_;
  CollectionGrouping::Grouping grouping_;
  PlaylistSequence playlist_sequence_;
  std::string playlist_filter_;
  std::string collection_text_filter_;
  PlaylistColumn sort_column_ = PlaylistColumn::Count;
  bool sort_descending_ = false;
  std::vector<int> selected_playlist_rows_;
  int playlist_menu_row_ = -1;
  std::string selection_playlist_name_;
  guint position_timeout_ = 0;
  guint analyzer_timeout_ = 0;
  int analyzer_timer_ms_ = 0;
  guint collection_filter_timeout_ = 0;
  std::vector<StreamingMetadataQueue::Entry> metadata_queue_;
  guint metadata_queue_timeout_ = 0;
  std::shared_ptr<bool> metadata_alive_ = std::make_shared<bool>(true);
  bool refreshing_devices_ = false;
  bool sponsor_prompted_ = false;
  bool was_maximized_ = false;
  bool was_minimized_ = false;
  bool playlists_loaded_ = false;
  bool has_pending_options_ = false;
  CommandlineOptions pending_options_;
  TaskbarProgress taskbar_;
#ifdef _WIN32
  std::unique_ptr<Windows7ThumbBar> thumbbar_;
  std::unique_ptr<WinSystemMediaTransportControls> smtc_;
#endif
#ifdef __APPLE__
  std::unique_ptr<MacSystemTrayIcon> macos_tray_;
#endif
};

#endif
