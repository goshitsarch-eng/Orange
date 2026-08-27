#include "config.h"
#include "core/logging.h"
#include "ui/mainwindow.h"

#include "collection/collectionbehaviour.h"
#include "collection/collectionincremental.h"
#include "playlist/playlistqueuerefresh.h"
#include "collection/collectionfilterkeyboard.h"
#include "collection/collectionfiltermenu.h"
#include "collection/collectionmenu.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectiongrouping.h"
#include "playlist/playlistsummary.h"
#include "collection/collectionsearchlabels.h"
#include "collection/collectionsearchsync.h"
#include "collection/collectionfilterwidget.h"
#include "collection/collectionviewcontainer.h"
#include "constants/backendsettings.h"
#include "constants/coverssettings.h"
#include "core/mainwindowsponsor.h"
#include "core/commandlinewindow.h"
#include "core/mainwindowsettings.h"
#include "core/playlistsloadedgate.h"
#include "dialogs/edittagloading.h"
#include "dialogs/edittagsave.h"
#include "dialogs/messagedialog.h"
#include "ui/mainwindowmenu.h"
#include "ui/mainwindowkeyboard.h"
#include "ui/mainwindowlook.h"
#include "ui/mainwindowsearchfocus.h"
#include "context/contextcover.h"
#include "context/contextview.h"
#include "covermanager/coverproviders.h"
#include "device/deviceviewcontainer.h"
#include "constants/moodbarsettings.h"
#include "moodbar/moodbarrenderer.h"
#include "moodbar/moodbarstyle.h"
#include "waveform/waveformrenderer.h"
#include "fileview/fileview.h"
#include "fileview/fileviewsongs.h"
#include "playlist/playlistcontainer.h"
#include "playlist/playlistfiltersync.h"
#include "playlist/playlistundolimits.h"
#include "playlist/playlistfolders.h"
#include "playlist/playlisttabnav.h"
#include "playlist/playlisttabmenu.h"
#include "playlist/playlistdragpayload.h"
#include "playlist/playlistlistdrop.h"
#include "radios/radiodrag.h"
#include "radios/radiomenu.h"
#include "radios/radiostreamplaylistitem.h"
#include "radios/radioviewcontainer.h"
#include "smartplaylists/smartplaylistsview.h"
#include "smartplaylists/smartplaylistsviewcontainer.h"
#include "playlist/playlistcolumnlayout.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistheadersort.h"
#include "playlist/playlisteditpolicy.h"
#include "playlist/playlistclipboard.h"
#include "playlist/playlisteditcolumn.h"
#include "playlist/playlistmenu.h"
#include "playlist/playliststopafter.h"
#include "core/playerstopafter.h"
#include "playlist/playlistlistcontainer.h"
#include "playlist/playlistlistlook.h"
#include "playlist/playlistsequence.h"
#include "queue/queuerows.h"
#include "queue/queueview.h"
#include "widgets/multiloadingindicator.h"
#include "widgets/multiloadingtext.h"
#include "ui/statusbarstack.h"
#include "widgets/playingwidget.h"
#include "widgets/seekbarmode.h"
#include "core/playeritemoptions.h"
#include "widgets/trackslider.h"
#include "widgets/tracksliderstate.h"
#include "widgets/tracksliderwheel.h"
#include "widgets/volumeslider.h"
#include "collection/collectiondirectory.h"
#include "constants/behavioursettings.h"
#include "constants/collectionsettings.h"
#include "constants/playlistsettings.h"
#include "dialogs/deletefilespolicy.h"
#include "streaming/streamingcollectionactions.h"
#include "streaming/streamingfavoriteaction.h"
#include "streaming/streamingsearchopts.h"
#include "constants/scrobblersettings.h"
#include "playlist/playlistrestorescroll.h"
#include "scrobbler/scrobblepoint.h"
#include "scrobbler/scrobbletoggleicon.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerlovestate.h"
#include "analyzer/analyzerframerate.h"
#include "constants/analyzersettings.h"
#include "constants/appearancesettings.h"
#include "playlist/playlistbehaviour.h"
#include "core/appearance.h"
#include "core/song.h"
#include "core/seekbarsettings.h"
#include "core/settings.h"
#include "core/windowgeometry.h"
#include "utilities/styleutils.h"
#include "playlist/playlistsaveoptionsdialog.h"
#include "playlistparsers/parserbase.h"
#include "device/cddasongloader.h"
#include "device/devicesongmenu.h"
#include "organize/organize.h"
#include "organize/organizedialog.h"
#include "organize/organizeformat.h"
#include "organize/organizeloading.h"
#include "smartplaylists/smartplaylist.h"
#include "streaming/streamingmetadatamerge.h"
#include "streaming/streamingmetadataqueue.h"
#ifdef HAVE_QOBUZ
#include "qobuz/qobuzservice.h"
#endif
#ifdef HAVE_SPOTIFY
#include "spotify/spotifyservice.h"
#endif
#include "streaming/streamingtabsview.h"
#include "core/urlhandler.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/trackselectiondialog.h"
#include "transcoder/transcodedialog.h"
#include "translations/translations.h"
#include "ui/dialogs.h"
#include "settings/settingspages.h"
#include "ui/settingsdialog.h"
#include "filterparser/filterparser.h"
#include "utilities/filefilters.h"
#include "utilities/filemanagerutils.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace {

GtkWidget *AppendStringRow(GtkListBox *list, const std::string &text, gpointer user_data, GDestroyNotify destroy = nullptr) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *label = gtk_label_new(text.c_str());
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_margin_start(label, 12);
  gtk_widget_set_margin_end(label, 12);
  gtk_widget_set_margin_top(label, 8);
  gtk_widget_set_margin_bottom(label, 8);
  gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
  g_object_set_data_full(G_OBJECT(row), "row-text", g_strdup(text.c_str()), g_free);
  if (user_data) {
    if (destroy) {
      g_object_set_data_full(G_OBJECT(row), "row-data", user_data, destroy);
    } else {
      g_object_set_data(G_OBJECT(row), "row-data", user_data);
    }
  }
  gtk_list_box_append(list, row);
  return row;
}

void ClearList(GtkWidget *list) {
  GtkWidget *child = gtk_widget_get_first_child(list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list), child);
    child = next;
  }
}

GtkWidget *MakeScrolledList(GtkWidget **list_out) {
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(scroll, TRUE);
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  *list_out = list;
  return scroll;
}

}  // namespace

MainWindow::MainWindow(AdwApplication *gtk_app, Application *app, const CommandlineOptions &options)
    : gtk_app_(gtk_app), app_(app) {
  grouping_ = CollectionGrouping::LoadCurrent();
  BuildUi();
  ConnectSignals();
  RefreshCollection();
  RefreshPlaylistsList();
  RefreshSmartPlaylists();
  RefreshPlaylist();
  RefreshRadio();
  RefreshDevices();
  RefreshFiles();
  RefreshStreaming();
  RefreshQueue();
  ApplyBehaviourSettings();
  pending_options_ = options;
  has_pending_options_ = true;
  if (app_->playlist_manager()->playlists_loaded()) {
    HandlePlaylistsLoaded();
  }
}

MainWindow::~MainWindow() {
  if (metadata_alive_) {
    *metadata_alive_ = false;
  }
  if (metadata_queue_timeout_) {
    g_source_remove(metadata_queue_timeout_);
    metadata_queue_timeout_ = 0;
  }
  SaveGeometry();
  if (position_timeout_) {
    g_source_remove(position_timeout_);
  }
  if (analyzer_timeout_) {
    g_source_remove(analyzer_timeout_);
  }
  if (collection_filter_timeout_) {
    g_source_remove(collection_filter_timeout_);
  }
  if (seekbar_fade_source_) {
    g_source_remove(seekbar_fade_source_);
    seekbar_fade_source_ = 0;
  }
  if (seekbar_menu_) {
    gtk_widget_unparent(seekbar_menu_);
    seekbar_menu_ = nullptr;
  }
}

void MainWindow::Present() {
  gtk_window_present(GTK_WINDOW(window_));
  if (app_ && app_->shortcuts()) {
    app_->shortcuts()->Raise();
  }
  MaybeShowSponsor();
}

void MainWindow::MaybeShowSponsor() {
  if (sponsor_prompted_) {
    return;
  }
  sponsor_prompted_ = true;
  Settings settings;
  if (!MainWindowSponsor::ShouldShowFromSettings(settings)) {
    return;
  }
  MessageDialog::Show(GTK_WINDOW(window_), MainWindowSponsor::Title(), MainWindowSponsor::Message(), MainWindowSponsor::DoNotShowAgain(), false,
                      [](bool do_not_show) {
                        if (do_not_show) {
                          Settings persist;
                          MainWindowSponsor::PersistDoNotShow(persist, true);
                        }
                      });
}

void MainWindow::HandlePlaylistsLoaded() {
  if (playlists_loaded_) {
    return;
  }
  playlists_loaded_ = true;
  if (has_pending_options_) {
    app_->ApplyCommandline(pending_options_);
    has_pending_options_ = false;
  }
  app_->player()->PlaylistsLoaded();
}

void MainWindow::CommandlineReceived(const CommandlineOptions &options) {
  if (PlaylistsLoadedGate::ShouldDeferCommandline(playlists_loaded_)) {
    pending_options_ = options;
    has_pending_options_ = true;
  } else {
    app_->ApplyCommandline(options);
  }
  if (CommandlineWindow::ShouldResize(options.player_action(), options.resize_width(), options.resize_height())) {
    gtk_window_set_default_size(GTK_WINDOW(window_), options.resize_width(), options.resize_height());
  }
  RefreshPlaylist();
  Present();
}

void MainWindow::BuildUi() {
  window_ = ADW_APPLICATION_WINDOW(adw_application_window_new(GTK_APPLICATION(gtk_app_)));
  gtk_window_set_title(GTK_WINDOW(window_), "Strawberry");
  gtk_widget_add_css_class(GTK_WIDGET(window_), "strawberry-main");
  RestoreGeometry();
  ApplyAppearance();

  GtkWidget *header = adw_header_bar_new();
  GtkWidget *menu_button = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
  GMenu *menu = g_menu_new();
  GMenu *music = g_menu_new();
  g_menu_append(music, Translations::Tr(MainWindowMenu::OpenFile()).c_str(), "win.open-files");
  g_menu_append(music, Translations::Tr(MainWindowMenu::AddFolder()).c_str(), "win.add-folder");
  g_menu_append(music, Translations::Tr(MainWindowMenu::OpenCD()).c_str(), "win.add-cd");
  g_menu_append(music, Translations::Tr(MainWindowMenu::AddStream()).c_str(), "win.add-stream");
  g_menu_append(music, Translations::Tr(MainWindowMenu::UpdateCollection()).c_str(), "win.rescan");
  g_menu_append(music, Translations::Tr(MainWindowMenu::FullScan()).c_str(), "win.full-scan");
  g_menu_append(music, Translations::Tr(MainWindowMenu::StopScan()).c_str(), "win.stop-scan");
  g_menu_append(music, Translations::Tr(MainWindowMenu::Mute()).c_str(), "win.mute");
  g_menu_append_section(menu, Translations::Tr("Music").c_str(), G_MENU_MODEL(music));
  GMenu *playlist = g_menu_new();
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::NewPlaylist()).c_str(), "win.new-playlist");
  g_menu_append(playlist, Translations::Tr("New playlist folder").c_str(), "win.new-playlist-folder");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::LoadPlaylist()).c_str(), "win.load-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::SavePlaylist()).c_str(), "win.save-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::SaveAllPlaylists()).c_str(), "win.save-all-playlists");
  g_menu_append(playlist, Translations::Tr("Rename playlist…").c_str(), "win.rename-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::ClosePlaylist()).c_str(), "win.close-playlist");
  g_menu_append(playlist, Translations::Tr("Delete playlist").c_str(), "win.delete-playlist");
  g_menu_append(playlist, Translations::Tr("Playlist columns…").c_str(), "win.playlist-columns");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::ClearPlaylist()).c_str(), "win.clear-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::ShufflePlaylist()).c_str(), "win.shuffle-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::RemoveDuplicates()).c_str(), "win.remove-duplicates");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::RemoveUnavailable()).c_str(), "win.remove-unavailable");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::RenumberTracks()).c_str(), "win.renumber-tracks");
  g_menu_append(playlist, Translations::Tr("Skip selected tracks").c_str(), "win.playlist-skip");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::JumpToPlaying()).c_str(), "win.jump-playing");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::GoNextTab()).c_str(), "win.next-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::GoPreviousTab()).c_str(), "win.previous-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::GoLastTab()).c_str(), "win.last-playlist");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::GoActiveTab()).c_str(), "win.active-playlist");
  g_menu_append(playlist, Translations::Tr("Rescan selected songs").c_str(), "win.rescan-selected");
  g_menu_append(playlist, Translations::Tr("Fetch streaming metadata").c_str(), "win.fetch-metadata");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::EditTrack()).c_str(), "win.edittag");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::CompleteTags()).c_str(), "win.autocomplete-tags");
  g_menu_append(playlist, Translations::Tr("Edit value").c_str(), "win.edit-value");
  g_menu_append(playlist, Translations::Tr(MainWindowMenu::SetValue()).c_str(), "win.set-column");
  g_menu_append(playlist, Translations::Tr("Undo").c_str(), "win.undo");
  g_menu_append(playlist, Translations::Tr("Redo").c_str(), "win.redo");
  g_menu_append(playlist, Translations::Tr("Smart playlist wizard…").c_str(), "win.smart-wizard");
  g_menu_append_section(menu, Translations::Tr("Playlist").c_str(), G_MENU_MODEL(playlist));
  GMenu *playback = g_menu_new();
  g_menu_append(playback, Translations::Tr("Play/Pause").c_str(), "win.play-pause");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::Stop()).c_str(), "win.stop");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::PreviousTrack()).c_str(), "win.previous-track");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::NextTrack()).c_str(), "win.next-track");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::StopAfter()).c_str(), "win.stop-after");
  g_menu_append(playback, Translations::Tr("Queue play next").c_str(), "win.queue-next");
  g_menu_append(playback, Translations::Tr("Scrobble current track").c_str(), "win.scrobble");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::ToggleScrobbling()).c_str(), "win.toggle-scrobbling");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::Love()).c_str(), "win.love");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::ShuffleMode()).c_str(), "win.shuffle-mode");
  g_menu_append(playback, Translations::Tr(MainWindowMenu::RepeatMode()).c_str(), "win.repeat-mode");
  g_menu_append_section(menu, Translations::Tr("Playback").c_str(), G_MENU_MODEL(playback));
  GMenu *tools = g_menu_new();
  g_menu_append(tools, Translations::Tr(MainWindowMenu::CoverManager()).c_str(), "win.covers");
  g_menu_append(tools, Translations::Tr("Cover search…").c_str(), "win.cover-search");
  g_menu_append(tools, Translations::Tr("Cover from URL…").c_str(), "win.cover-from-url");
  g_menu_append(tools, Translations::Tr("Export covers…").c_str(), "win.cover-export");
  g_menu_append(tools, Translations::Tr(MainWindowMenu::Equalizer()).c_str(), "win.equalizer");
  g_menu_append(tools, Translations::Tr(MainWindowMenu::Transcode()).c_str(), "win.transcode");
  g_menu_append(tools, Translations::Tr("Add selection to transcoder…").c_str(), "win.transcode-selected");
  g_menu_append(tools, Translations::Tr("Organize files…").c_str(), "win.organize");
  g_menu_append(tools, Translations::Tr("Copy to collection").c_str(), "win.copy-collection");
  g_menu_append(tools, Translations::Tr("Move to collection").c_str(), "win.move-collection");
  g_menu_append(tools, Translations::Tr("Copy to device…").c_str(), "win.copy-device");
  g_menu_append(tools, Translations::Tr("Show in collection").c_str(), "win.show-in-collection");
  g_menu_append(tools, Translations::Tr("Open in file manager").c_str(), "win.open-file-manager");
  g_menu_append(tools, Translations::Tr("Copy song URL").c_str(), "win.copy-url");
  g_menu_append(tools, Translations::Tr("Delete files…").c_str(), "win.delete-files");
  g_menu_append(tools, Translations::Tr("Fetch tags…").c_str(), "win.tagfetch");
  g_menu_append(tools, Translations::Tr("Edit tags…").c_str(), "win.edittag");
  GMenu *rate = g_menu_new();
  g_menu_append(rate, Translations::Tr("1 star").c_str(), "win.rate(1)");
  g_menu_append(rate, Translations::Tr("2 stars").c_str(), "win.rate(2)");
  g_menu_append(rate, Translations::Tr("3 stars").c_str(), "win.rate(3)");
  g_menu_append(rate, Translations::Tr("4 stars").c_str(), "win.rate(4)");
  g_menu_append(rate, Translations::Tr("5 stars").c_str(), "win.rate(5)");
  g_menu_append_submenu(tools, Translations::Tr("Rate").c_str(), G_MENU_MODEL(rate));
  g_menu_append(tools, Translations::Tr("Collection grouping…").c_str(), "win.group-by");
  g_menu_append(tools, Translations::Tr("Cycle analyzer").c_str(), "win.cycle-analyzer");
  g_menu_append(tools, Translations::Tr(MainWindowMenu::Console()).c_str(), "win.console");
  g_menu_append(tools, Translations::Tr(MainWindowMenu::ShowSidebar()).c_str(), "win.show-sidebar");
  g_menu_append_section(menu, Translations::Tr("Tools").c_str(), G_MENU_MODEL(tools));
  GMenu *appmenu = g_menu_new();
  g_menu_append(appmenu, Translations::Tr(MainWindowMenu::Settings()).c_str(), "win.preferences");
  g_menu_append(appmenu, Translations::Tr("Keyboard shortcuts").c_str(), "win.shortcuts");
  g_menu_append(appmenu, Translations::Tr(MainWindowMenu::About()).c_str(), "win.about");
  g_menu_append(appmenu, Translations::Tr(MainWindowMenu::Quit()).c_str(), "win.quit");
  g_menu_append_section(menu, nullptr, G_MENU_MODEL(appmenu));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));
  adw_header_bar_pack_end(ADW_HEADER_BAR(header), menu_button);

  collection_search_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(collection_search_),
                                        Translations::CStr(CollectionSearchLabels::Placeholder()));
  gtk_widget_set_tooltip_text(collection_search_, Translations::CStr(FilterParser::ToolTip().c_str()));
  adw_header_bar_pack_start(ADW_HEADER_BAR(header), collection_search_);
  g_signal_connect(collection_search_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     const int length = text ? static_cast<int>(g_utf8_strlen(text, -1)) : 0;
                     const int songs = self->collection_container_ ? self->collection_container_->view()->model()->TotalSongs() : 0;
                     if (self->collection_filter_timeout_) {
                       g_source_remove(self->collection_filter_timeout_);
                       self->collection_filter_timeout_ = 0;
                     }
                     if (CollectionFilterMenu::ShouldDelay(CollectionFilterMenu::DelayBehaviour::DelayedOnLargeLibraries, length, songs)) {
                       self->collection_filter_timeout_ = g_timeout_add(CollectionFilterMenu::kFilterDelayMs, [](gpointer data) -> gboolean {
                         auto *self = static_cast<MainWindow *>(data);
                         self->collection_filter_timeout_ = 0;
                         if (self->collection_search_) {
                           self->RefreshCollection(gtk_editable_get_text(GTK_EDITABLE(self->collection_search_)), true);
                         }
                         return G_SOURCE_REMOVE;
                       }, self);
                       return;
                     }
                     self->RefreshCollection(text ? text : "", true);
                   }),
                   this);
  g_signal_connect(collection_search_, "activate", G_CALLBACK(+[](GtkSearchEntry *, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     if (self->collection_container_) {
                       self->collection_container_->view()->FilterReturnPressed();
                     }
                   }),
                   this);
  GtkEventController *search_keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(collection_search_, search_keys);
  g_signal_connect(search_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     auto *self = static_cast<MainWindow *>(data);
                     if (!self->collection_container_) {
                       return FALSE;
                     }
                     const CollectionFilterKeyboard::Action action = CollectionFilterKeyboard::FromSearchKey(keyval);
                     if (action == CollectionFilterKeyboard::Action::MoveUp || action == CollectionFilterKeyboard::Action::MoveDown) {
                       self->collection_container_->view()->FocusAndMove(CollectionFilterKeyboard::SearchMoveAction(action));
                       return TRUE;
                     }
                     if (action == CollectionFilterKeyboard::Action::Clear) {
                       gtk_editable_set_text(GTK_EDITABLE(self->collection_search_), "");
                       return TRUE;
                     }
                     return FALSE;
                   })),
                   this);

  GtkWidget *toolbar = adw_toolbar_view_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);

  toast_overlay_ = ADW_TOAST_OVERLAY(adw_toast_overlay_new());
  split_view_ = adw_overlay_split_view_new();
  adw_overlay_split_view_set_sidebar_width_fraction(ADW_OVERLAY_SPLIT_VIEW(split_view_), 0.30);

  BuildSidebar();
  GtkWidget *sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *switcher = adw_view_switcher_new();
  gtk_widget_add_css_class(switcher, "strawberry-tabbar");
  gtk_widget_add_css_class(sidebar_box, "strawberry-left-panel");
  adw_view_switcher_set_policy(ADW_VIEW_SWITCHER(switcher), ADW_VIEW_SWITCHER_POLICY_NARROW);
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), sidebar_stack_);
  gtk_box_append(GTK_BOX(sidebar_box), switcher);
  gtk_box_append(GTK_BOX(sidebar_box), GTK_WIDGET(sidebar_stack_));
  gtk_widget_set_vexpand(GTK_WIDGET(sidebar_stack_), TRUE);
  adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(split_view_), sidebar_box);

  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  BuildPlaylist();
  gtk_box_append(GTK_BOX(content), playlist_container_->widget());
  BuildPlayerBar();
  gtk_box_append(GTK_BOX(content), GTK_WIDGET(g_object_get_data(G_OBJECT(play_button_), "player-box")));
  adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(split_view_), content);

  adw_toast_overlay_set_child(toast_overlay_, split_view_);
  ApplySidebar();
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(toast_overlay_));
  adw_application_window_set_content(window_, toolbar);
  g_signal_connect(window_, "close-request", G_CALLBACK(+[](GtkWindow *, gpointer data) -> gboolean {
                     auto *self = static_cast<MainWindow *>(data);
                     self->SaveGeometry();
                     Settings settings;
                     settings.BeginGroup(BehaviourSettings::kSettingsGroup);
                     if (settings.BoolValue(BehaviourSettings::kKeepRunning, BehaviourSettings::kDefaultKeepRunning) &&
                         self->app_->tray()->available()) {
                       gtk_widget_set_visible(GTK_WIDGET(self->window_), FALSE);
                       return TRUE;
                     }
                     self->app_->Exit();
                     return FALSE;
                   }),
                   this);
  app_->tray()->ShowHide.Connect([this]() {
    if (gtk_widget_get_visible(GTK_WIDGET(window_))) {
      gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
    } else {
      Present();
    }
  });
  app_->tray()->Quit.Connect([this]() {
    app_->Exit();
    gtk_window_close(GTK_WINDOW(window_));
  });

  auto add_action = [this](const char *name, GCallback callback) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_signal_connect(action, "activate", callback, this);
    g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(action));
  };
  add_action("preferences", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->OpenSettings(); }));
  add_action("about", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->OpenAbout(); }));
  add_action("quit", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->app_->Exit();
               gtk_window_close(GTK_WINDOW(self->window_));
             }));
  add_action("open-files", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->AddFiles(); }));
  add_action("add-folder", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->AddCollectionFolder(); }));
  add_action("add-cd", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->AddCdTracks(); }));
  add_action("rescan", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RescanCollection(false); }));
  add_action("full-scan", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RescanCollection(true); }));
  add_action("stop-scan", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->app_->collection()->AbortScan();
               self->ShowToast("Collection scan stopping");
             }));
  add_action("new-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->NewPlaylist(); }));
  add_action("new-playlist-folder", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->NewPlaylistFolder(); }));
  add_action("load-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->LoadPlaylistFile(); }));
  add_action("save-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->SavePlaylistFile(); }));
  add_action("rename-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RenameCurrentPlaylist(); }));
  add_action("close-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CloseCurrentPlaylist(); }));
  add_action("delete-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->DeleteCurrentPlaylist(); }));
  add_action("clear-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->ClearPlaylist(); }));
  add_action("shuffle-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->ShuffleCurrent(); }));
  add_action("remove-duplicates", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RemoveDuplicates(); }));
  add_action("remove-unavailable", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RemoveUnavailable(); }));
  add_action("renumber-tracks", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RenumberTracks(); }));
  add_action("stop-after", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->StopAfterCurrent(); }));
  add_action("scrobble", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->ScrobbleCurrent(); }));
  add_action("toggle-scrobbling", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (self->app_ && self->app_->scrobbler()) {
                 self->app_->scrobbler()->ToggleScrobbling();
               }
               self->UpdateScrobblerButtons();
             }));
  add_action("shuffle-mode", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CycleShuffle(); }));
  add_action("repeat-mode", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CycleRepeat(); }));
  add_action("love", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               MainWindow::OnLove(nullptr, data);
             }));
  add_action("queue-next", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->QueuePlayNext(); }));
  add_action("transcode-selected", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->AddSelectedToTranscoder(); }));
  add_action("copy-collection", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CopySelectedToCollection(false); }));
  add_action("move-collection", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CopySelectedToCollection(true); }));
  add_action("show-in-collection", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->ShowInCollection(); }));
  add_action("open-file-manager", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->OpenSelectedInFileManager(); }));
  add_action("copy-url", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CopySelectedUrl(); }));
  add_action("rate-5", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RateSelected(5); }));
  {
    GSimpleAction *rate = g_simple_action_new("rate", G_VARIANT_TYPE_INT32);
    g_signal_connect(rate, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                       static_cast<MainWindow *>(data)->RateSelected(g_variant_get_int32(param));
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(rate));
  }
  add_action("edit-value", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->EditColumnValue(); }));
  add_action("set-column", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->SetColumnTo(); }));
  add_action("focus-search", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->FocusSearchField(); }));
  add_action("playlist-skip", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->SkipSelected(); }));
  add_action("jump-playing", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->JumpToPlaying(); }));
  add_action("next-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->NextPlaylistTab(); }));
  add_action("previous-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->PreviousPlaylistTab(); }));
  add_action("last-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->LastPlaylistTab(); }));
  add_action("active-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->ActivePlaylistTab(); }));
  {
    Settings settings;
    settings.BeginGroup(MainWindowSettings::kSettingsGroup);
    const bool show = settings.BoolValue(MainWindowSettings::kShowSidebar, MainWindowSettings::kDefaultShowSidebar);
    sidebar_action_ = g_simple_action_new_stateful("show-sidebar", nullptr, g_variant_new_boolean(show));
    g_signal_connect(sidebar_action_, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *, gpointer data) {
                       GVariant *state = g_action_get_state(G_ACTION(action));
                       const bool current = state && g_variant_get_boolean(state);
                       if (state) {
                         g_variant_unref(state);
                       }
                       static_cast<MainWindow *>(data)->SetShowSidebar(!current);
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(sidebar_action_));
  }
  mute_action_ = g_simple_action_new_stateful("mute", nullptr, g_variant_new_boolean(false));
  g_signal_connect(mute_action_, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
                     static_cast<MainWindow *>(data)->ToggleMute();
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(mute_action_));
  ApplyMuteUi(app_->player()->GetVolume());
  add_action("rescan-selected", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RescanSelected(); }));
  add_action("fetch-metadata", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->FetchStreamingMetadata(); }));
  add_action("autocomplete-tags", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->AutoCompleteTags(); }));
  {
    GSimpleAction *add_to = g_simple_action_new("add-to-playlist", G_VARIANT_TYPE_INT32);
    g_signal_connect(add_to, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                       static_cast<MainWindow *>(data)->AddSelectedToPlaylist(g_variant_get_int32(param));
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(add_to));
  }
  add_action("add-stream", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::AddStream(GTK_WINDOW(self->window_), [self](const std::string &, const std::string &url) {
                 self->app_->playlist_manager()->InsertUrls({url});
               });
             }));
  add_action("covers", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverManager(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("cover-search", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverSearch(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("cover-from-url", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverFromUrl(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("cover-export", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverExport(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("copy-device", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CopyToDevice(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("playlist-copy-device", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::CopyToDevice(GTK_WINDOW(self->window_), self->app_, self->SelectedSongs());
             }));
  add_action("delete-files", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::DeleteFiles(GTK_WINDOW(self->window_), self->app_, self->SelectedSongs(), DeleteFilesPolicy::Source::Playlist);
             }));
  add_action("save-all-playlists", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::SaveAllPlaylists(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("playlist-columns", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::PlaylistColumns(GTK_WINDOW(self->window_), [self]() { self->RefreshPlaylist(); });
             }));
  add_action("equalizer", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Equalizer(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("transcode", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Transcode(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("organize", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Organize(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("organize-selected", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::Organize(GTK_WINDOW(self->window_), self->app_, self->SelectedSongs(), true);
             }));
  add_action("stop", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Stop(); }));
  add_action("play-pause", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->PlayPause(); }));
  add_action("previous-track", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Previous(); }));
  add_action("next-track", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Next(); }));
  add_action("collection-append", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::Append(play, self->EngineStopped()), self->CollectionSongs());
             }));
  add_action("collection-new", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::OpenInNew(play, self->EngineStopped()), self->CollectionSongs());
             }));
  add_action("collection-replace", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::Replace(play, self->EngineStopped()), self->CollectionSongs());
             }));
  add_action("streaming-append", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::Append(play, self->EngineStopped()), self->streaming_menu_songs_);
             }));
  add_action("streaming-replace", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::Replace(play, self->EngineStopped()), self->streaming_menu_songs_);
             }));
  add_action("streaming-new", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::OpenInNew(play, self->EngineStopped()), self->streaming_menu_songs_);
             }));
  add_action("streaming-enqueue", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->ApplyCollectionPlan(CollectionBehaviour::Enqueue(), self->streaming_menu_songs_);
             }));
  add_action("streaming-enqueue-next", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->ApplyCollectionPlan(CollectionBehaviour::EnqueueNext(), self->streaming_menu_songs_);
             }));
  add_action("streaming-favorite", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->StreamingFavorite(true);
             }));
  add_action("streaming-unfavorite", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->StreamingFavorite(false);
             }));
  add_action("streaming-add-artists", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->StreamingAddToList(StreamingCollectionStore::List::Artists);
             }));
  add_action("streaming-add-albums", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->StreamingAddToList(StreamingCollectionStore::List::Albums);
             }));
  add_action("streaming-search", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->StreamingSearchForThis();
             }));
  add_action("streaming-add-songs", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->StreamingAddToList(StreamingCollectionStore::List::Songs);
             }));
  add_action("streaming-configure", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->OpenSettings(SettingsPages::ForService(self->streaming_service_name_));
             }));
  add_action("radio-append", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::Append(play, self->EngineStopped()), self->radio_menu_songs_);
             }));
  add_action("radio-replace", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::Replace(play, self->EngineStopped()), self->radio_menu_songs_);
             }));
  add_action("radio-new", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Settings settings;
               settings.BeginGroup(BehaviourSettings::kSettingsGroup);
               const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
                   settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
               self->ApplyCollectionPlan(CollectionBehaviour::OpenInNew(play, self->EngineStopped()), self->radio_menu_songs_);
             }));
  add_action("radio-enqueue", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->ApplyCollectionPlan(CollectionBehaviour::Enqueue(), self->radio_menu_songs_);
             }));
  add_action("radio-homepage", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               for (const std::string &url : RadioMenu::UniqueUrls(self->radio_menu_channels_, false)) {
                 g_app_info_launch_default_for_uri(url.c_str(), nullptr, nullptr);
               }
             }));
  add_action("radio-donate", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               for (const std::string &url : RadioMenu::UniqueUrls(self->radio_menu_channels_, true)) {
                 g_app_info_launch_default_for_uri(url.c_str(), nullptr, nullptr);
               }
             }));
  add_action("radio-refresh", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (self->radio_container_) {
                 self->radio_container_->RefreshChannels();
               }
             }));
  add_action("playlist-list-open", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->SelectPlaylistByName(self->playlist_list_menu_name_);
             }));
  add_action("playlist-list-favorite", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (Playlist *playlist = self->PlaylistByName(self->playlist_list_menu_name_)) {
                 self->app_->playlist_manager()->Favorite(playlist->id(), !playlist->favorite());
                 self->RefreshPlaylistsList();
               }
             }));
  add_action("playlist-list-rename", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->SelectPlaylistByName(self->playlist_list_menu_name_);
               self->RenameCurrentPlaylist();
             }));
  add_action("playlist-list-delete", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->SelectPlaylistByName(self->playlist_list_menu_name_);
               self->DeleteCurrentPlaylist();
             }));
  add_action("playlist-list-save", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->SelectPlaylistByName(self->playlist_list_menu_name_);
               self->SavePlaylistFile();
             }));
  add_action("playlist-list-copy", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->SelectPlaylistByName(self->playlist_list_menu_name_);
               Dialogs::CopyToDevice(GTK_WINDOW(self->window_), self->app_);
             }));
  add_action("playlist-list-new-folder", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->NewPlaylistFolder(); }));
  add_action("playlist-list-rename-folder", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->RenamePlaylistFolder(self->playlist_list_menu_folder_);
             }));
  add_action("playlist-list-delete-folder", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->DeletePlaylistFolder(self->playlist_list_menu_folder_);
             }));
  add_action("playlist-list-remove-from-folder", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->MovePlaylistToFolder(self->playlist_list_menu_name_, {});
             }));
  add_action("collection-expand-all", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (self->collection_container_) {
                 self->collection_container_->view()->ExpandAll();
               }
             }));
  add_action("collection-collapse-all", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (self->collection_container_) {
                 self->collection_container_->view()->CollapseAll();
               }
             }));
  add_action("collection-configure", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->OpenSettings(SettingsPages::Collection());
             }));
  add_action("collection-various-on", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->ForceCompilationSelected(true);
             }));
  add_action("collection-various-off", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->ForceCompilationSelected(false);
             }));
  add_action("collection-rescan", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               const SongList songs = self->CollectionSongs();
               if (songs.empty()) {
                 return;
               }
               self->app_->collection()->Rescan(songs);
               self->ShowToast("Rescanned " + std::to_string(songs.size()) + " song(s)");
             }));
  add_action("collection-enqueue", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->ApplyCollectionPlan(CollectionBehaviour::Enqueue(), self->CollectionSongs());
             }));
  add_action("collection-enqueue-next", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               self->ApplyCollectionPlan(CollectionBehaviour::EnqueueNext(), self->CollectionSongs());
             }));
  add_action("collection-search", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (!self->collection_container_) {
                 return;
               }
               const CollectionItem *item = self->collection_container_->view()->SelectedItem();
               const std::string query = CollectionBehaviour::SearchQuery(item, self->collection_container_->view()->model()->grouping());
               if (query.empty()) {
                 return;
               }
               adw_view_stack_set_visible_child_name(self->sidebar_stack_, "collection");
               self->RefreshCollection(query, true);
             }));
  add_action("collection-organize", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::Organize(GTK_WINDOW(self->window_), self->app_, self->CollectionSongs());
             }));
  add_action("collection-copy-device", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::CopyToDevice(GTK_WINDOW(self->window_), self->app_, self->CollectionSongs());
             }));
  {
    GSimpleAction *preset = g_simple_action_new("collection-preset", G_VARIANT_TYPE_INT32);
    g_signal_connect(preset, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       const int index = g_variant_get_int32(param);
                       const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
                       if (index < 0 || static_cast<size_t>(index) >= presets.size() || presets[static_cast<size_t>(index)].advanced) {
                         return;
                       }
                       self->grouping_ = presets[static_cast<size_t>(index)].grouping;
                       CollectionGrouping::SaveCurrent(self->grouping_);
                       if (self->collection_container_) {
                         self->collection_container_->filter_widget()->SetGrouping(self->grouping_);
                       }
                       self->RefreshCollection();
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(preset));
  }
  add_action("collection-edittag", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::EditTag(GTK_WINDOW(self->window_), self->app_, self->CollectionSongs());
             }));
  add_action("collection-browse", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               const SongList songs = self->CollectionSongs();
               if (songs.empty() || !FileManagerUtils::OpenInFileManager(FileUtils::PathFromUri(songs.front().url()))) {
                 self->ShowToast("Could not open the file manager");
               }
             }));
  add_action("collection-delete", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::DeleteFiles(GTK_WINDOW(self->window_), self->app_, self->CollectionSongs(), DeleteFilesPolicy::Source::Collection);
             }));
  add_action("tagfetch", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::TagFetcher(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("edittag", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               SongList songs;
               std::vector<int> rows;
               if (Playlist *playlist = self->app_->playlist_manager()->current()) {
                 EditTagSave::CollectPlaylistSelection(playlist->songs(), self->SelectedPlaylistRows(), &songs, &rows);
               }
               Dialogs::EditTag(GTK_WINDOW(self->window_), self->app_, songs, rows);
             }));
  add_action("shortcuts", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Shortcuts(GTK_WINDOW(static_cast<MainWindow *>(data)->window_)); }));
  add_action("undo", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->UndoPlaylist(); }));
  add_action("redo", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->RedoPlaylist(); }));
  add_action("smart-wizard", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::SmartPlaylistWizard(GTK_WINDOW(self->window_), self->app_);
               self->RefreshPlaylistsList();
               self->RefreshPlaylist();
             }));
  add_action("group-by", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::GroupBy(GTK_WINDOW(self->window_), self->grouping_, [self](const CollectionGrouping::Grouping &grouping) {
                 self->grouping_ = grouping;
                 CollectionGrouping::SaveCurrent(grouping);
                 if (self->collection_container_) {
                   self->collection_container_->filter_widget()->SetGrouping(grouping);
                 }
                 self->RefreshCollection();
               });
             }));
  add_action("console", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::Console(GTK_WINDOW(self->window_), self->app_);
             }));
  add_action("cycle-analyzer", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CycleAnalyzer(); }));
  add_action("playlist-play", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->PlayPlaylistMenuRow();
             }));
  add_action("playlist-stop-after", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               static_cast<MainWindow *>(data)->PlaylistStopAfter();
             }));
  add_action("playlist-queue", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Playlist *playlist = self->app_->playlist_manager()->current();
               const std::vector<int> rows = self->SelectedPlaylistRows();
               if (!playlist || rows.empty()) {
                 return;
               }
               bool all_queued = true;
               for (int row : rows) {
                 if (!self->app_->queue()->ContainsPlaylistRow(playlist->id(), row)) {
                   all_queued = false;
                   break;
                 }
               }
               for (int row : rows) {
                 if (row < 0 || row >= playlist->row_count()) {
                   continue;
                 }
                 if (all_queued) {
                   self->app_->queue()->TogglePlaylistRow(playlist->id(), row, playlist->song(row));
                 } else if (!self->app_->queue()->ContainsPlaylistRow(playlist->id(), row)) {
                   self->app_->queue()->Append(playlist->song(row), playlist->id(), row);
                 }
               }
               self->RefreshQueue();
               self->RefreshPlaylist();
               self->ShowToast(all_queued ? "Removed from queue" : "Added to queue");
             }));
  add_action("playlist-remove", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (Playlist *playlist = self->app_->playlist_manager()->current()) {
                 const std::vector<int> rows = self->SelectedPlaylistRows();
                 self->app_->queue()->RemapAfterPlaylistRemove(playlist->id(), rows);
                 playlist->RemoveRows(rows);
                 self->selected_playlist_rows_.clear();
                 self->app_->playlist_manager()->SaveCurrent();
                 self->RefreshPlaylist();
                 self->RefreshQueue();
               }
             }));
  auto set_accels = [this](const char *action, const char *accel) {
    const char *accels[] = {accel, nullptr};
    gtk_application_set_accels_for_action(GTK_APPLICATION(gtk_app_), action, accels);
  };
  set_accels("win.edit-value", "F2");
  set_accels("win.focus-search", "<Control>f");
  set_accels("win.undo", "<Control>z");
  set_accels("win.redo", "<Control><Shift>z");
  set_accels("win.new-playlist", "<Control>n");
  const char *open_accels[] = {MainWindowKeyboard::OpenFilesAccel(), MainWindowKeyboard::AddFileAccel(), nullptr};
  gtk_application_set_accels_for_action(GTK_APPLICATION(gtk_app_), "win.open-files", open_accels);
  set_accels("win.save-playlist", "<Control>s");
  const char *pref_accels[] = {MainWindowKeyboard::SettingsCommaAccel(), MainWindowKeyboard::SettingsAccel(), nullptr};
  gtk_application_set_accels_for_action(GTK_APPLICATION(gtk_app_), "win.preferences", pref_accels);
  set_accels("win.quit", "<Control>q");
  const char *next_accels[] = {"<Control>Tab", "<Control>Page_Down", nullptr};
  gtk_application_set_accels_for_action(GTK_APPLICATION(gtk_app_), "win.next-playlist", next_accels);
  const char *prev_accels[] = {"<Control><Shift>Tab", "<Control>Page_Up", nullptr};
  gtk_application_set_accels_for_action(GTK_APPLICATION(gtk_app_), "win.previous-playlist", prev_accels);
  set_accels("win.last-playlist", "<Control>9");
  set_accels("win.active-playlist", "<Control><Shift>p");
  set_accels("win.mute", MainWindowLook::MuteAccel());
  set_accels("win.close-playlist", MainWindowLook::ClosePlaylistAccel());
  set_accels("win.playlist-queue", MainWindowLook::PlaylistQueueAccel());
  set_accels("win.queue-next", MainWindowLook::QueuePlayNextAccel());
  set_accels("win.play-pause", MainWindowKeyboard::PlayPauseAccel());
  set_accels("win.stop", MainWindowKeyboard::StopAccel());
  set_accels("win.previous-track", MainWindowKeyboard::PreviousAccel());
  set_accels("win.next-track", MainWindowKeyboard::NextAccel());
  set_accels("win.about", MainWindowKeyboard::AboutAccel());
  set_accels("win.love", MainWindowKeyboard::LoveAccel());
  set_accels("win.stop-after", MainWindowKeyboard::StopAfterAccel());
  set_accels("win.shuffle-playlist", MainWindowKeyboard::ShuffleAccel());
  set_accels("win.jump-playing", MainWindowKeyboard::JumpAccel());
  set_accels("win.load-playlist", MainWindowKeyboard::LoadPlaylistAccel());
  set_accels("win.autocomplete-tags", MainWindowKeyboard::AutoCompleteTagsAccel());
  set_accels("win.transcode-selected", MainWindowKeyboard::TranscodeSelectedAccel());
  set_accels("win.clear-playlist", MainWindowKeyboard::ClearPlaylistAccel());
  set_accels("win.edittag", MainWindowKeyboard::EditTrackAccel());

  GtkEventController *capture_keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(capture_keys, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(GTK_WIDGET(window_), capture_keys);
  g_signal_connect(capture_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<MainWindow *>(data)->OnWindowKeyCapture(keyval);
                   })),
                   this);
  GtkEventController *bubble_keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(GTK_WIDGET(window_), bubble_keys);
  g_signal_connect(bubble_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<MainWindow *>(data)->OnWindowKeyBubble(keyval);
                   })),
                   this);
}

void MainWindow::BuildSidebar() {
  sidebar_stack_ = ADW_VIEW_STACK(adw_view_stack_new());
  gtk_widget_set_vexpand(GTK_WIDGET(sidebar_stack_), TRUE);

  BuildContext();
  adw_view_stack_add_titled_with_icon(sidebar_stack_, context_view_->widget(), "context", "Context", "audio-x-generic-symbolic");
  GtkWidget *collection_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  collection_container_ = std::make_unique<CollectionViewContainer>();
  collection_container_->view()->SetCoverLoader(app_->albumcover_loader());
  gtk_widget_set_vexpand(collection_container_->widget(), TRUE);
  collection_container_->filter_widget()->SetChangedCallback([this]() {
    const bool enabled = collection_container_->filter_widget()->options().TextSearchEnabled();
    if (collection_search_) {
      gtk_widget_set_sensitive(collection_search_, enabled);
      if (!enabled) {
        gtk_editable_set_text(GTK_EDITABLE(collection_search_), "");
      }
    }
    if (!enabled) {
      collection_text_filter_.clear();
    }
    RefreshCollection();
  });
  collection_container_->filter_widget()->SetGrouping(grouping_);
  collection_container_->filter_widget()->SetGroupingChangedCallback([this](const CollectionGrouping::Grouping &grouping) {
    grouping_ = grouping;
    CollectionGrouping::SaveCurrent(grouping);
    RefreshCollection();
  });
  collection_container_->filter_widget()->SetMenuActionCallback([this](CollectionFilterMenu::ActionKind kind) {
    auto apply = [this](const CollectionGrouping::Grouping &grouping) {
      grouping_ = grouping;
      CollectionGrouping::SaveCurrent(grouping);
      if (collection_container_) {
        collection_container_->filter_widget()->SetGrouping(grouping);
      }
      RefreshCollection();
    };
    if (kind == CollectionFilterMenu::ActionKind::Advanced) {
      Dialogs::GroupBy(GTK_WINDOW(window_), grouping_, apply);
    } else if (kind == CollectionFilterMenu::ActionKind::Manage) {
      Dialogs::ManageSavedGroupings(GTK_WINDOW(window_), apply);
      if (collection_container_) {
        collection_container_->filter_widget()->ReloadMenu();
      }
    } else if (kind == CollectionFilterMenu::ActionKind::Configure) {
      OpenSettings(SettingsPages::Collection());
    }
  });
  collection_container_->view()->SetActivateCallback([this](const SongList &songs) {
    Settings settings;
    settings.BeginGroup(BehaviourSettings::kSettingsGroup);
    const auto add = static_cast<BehaviourSettings::AddBehaviour>(
        settings.IntValue(BehaviourSettings::kDoubleClickAddMode, static_cast<int>(BehaviourSettings::kDefaultDoubleClickAddMode)));
    const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
        settings.IntValue(BehaviourSettings::kDoubleClickPlayMode, static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlayMode)));
    ApplyCollectionPlan(CollectionBehaviour::FromDoubleClick(add, play, EngineStopped()), songs);
  });
  collection_container_->view()->SetEnqueueCallback([this](const SongList &songs) {
    ApplyCollectionPlan(CollectionBehaviour::Enqueue(), songs);
  });
  collection_container_->view()->SetEmptyCallback([this]() { OpenSettings(SettingsPages::Collection()); });
  collection_container_->view()->SetFocusFilterCallback([this]() { FocusCollectionSearch(); });
  collection_container_->view()->SetMenuCallback([this](double, double) { ShowCollectionMenu(); });
  gtk_box_append(GTK_BOX(collection_page), collection_container_->widget());
  adw_view_stack_add_titled_with_icon(sidebar_stack_, collection_page, "collection", "Collection",
                                      "media-optical-cd-audio-symbolic");
  playlist_list_container_ = std::make_unique<PlaylistListContainer>();
  playlist_list_container_->SetActivateCallback([this](const std::string &name) { SelectPlaylistByName(name); });
  playlist_list_container_->SetNewCallback([this] { NewPlaylist(); });
  playlist_list_container_->SetNewFolderCallback([this] { NewPlaylistFolder(); });
  playlist_list_container_->SetDeleteCallback([this](const std::string &name) {
    SelectPlaylistByName(name);
    DeleteCurrentPlaylist();
  });
  playlist_list_container_->SetDeleteFolderCallback([this](const std::string &path) { DeletePlaylistFolder(path); });
  playlist_list_container_->SetSaveCallback([this](const std::string &name) {
    SelectPlaylistByName(name);
    SavePlaylistFile();
  });
  playlist_list_container_->SetCopyCallback([this](const std::string &name) {
    SelectPlaylistByName(name);
    Dialogs::CopyToDevice(GTK_WINDOW(window_), app_, {}, name);
  });
  playlist_list_container_->SetMenuCallback([this](const std::string &name) { ShowPlaylistListMenu(name); });
  playlist_list_container_->SetFavoriteCallback([this](const std::string &name, bool on) {
    for (const auto &playlist : app_->playlist_manager()->playlists()) {
      if (playlist->name() == name) {
        app_->playlist_manager()->Favorite(playlist->id(), on);
        break;
      }
    }
  });
  playlist_list_container_->SetDropCallback(
      [this](const std::string &name, const std::string &payload, bool folder) { DropOnPlaylistList(name, payload, folder); });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, playlist_list_container_->widget(), "playlists", "Playlists",
                                      "view-list-symbolic");
  smart_container_ = std::make_unique<SmartPlaylistsViewContainer>();
  smart_container_->SetActivateCallback([this](const SmartPlaylistsItem &item) {
    if (item.kind == SmartPlaylistsItem::Kind::Wizard) {
      Dialogs::SmartPlaylistWizard(GTK_WINDOW(window_), app_);
      smart_container_->Reload();
      RefreshPlaylistsList();
      RefreshPlaylist();
      return;
    }
    RunSmartPlaylist(item.key);
  });
  smart_container_->SetDeleteCallback([this](const SmartPlaylistsItem &item) {
    if (item.kind != SmartPlaylistsItem::Kind::Saved) {
      return;
    }
    SmartPlaylistSearch::RemoveSaved(item.title);
    smart_container_->Reload();
  });
  smart_container_->SetSongsCallback([this](const SmartPlaylistsItem &item) { return item.search.Search(app_->collection()->Songs()); });
  smart_container_->SetActionCallback([this](const SmartPlaylistsItem &item, SmartPlaylistsAction action) {
    switch (action) {
      case SmartPlaylistsAction::Activate:
        RunSmartPlaylist(item.key);
        break;
      case SmartPlaylistsAction::Append:
        app_->playlist_manager()->PlaySmartPlaylist(item.key, false, false);
        RefreshPlaylistsList();
        RefreshPlaylist();
        break;
      case SmartPlaylistsAction::Replace:
        app_->playlist_manager()->PlaySmartPlaylist(item.key, false, true);
        RefreshPlaylistsList();
        RefreshPlaylist();
        break;
      case SmartPlaylistsAction::OpenInNew:
        app_->playlist_manager()->PlaySmartPlaylist(item.key, true, true);
        RefreshPlaylistsList();
        RefreshPlaylist();
        break;
      case SmartPlaylistsAction::Queue: {
        const SongList songs = item.search.Search(app_->collection()->Songs());
        for (const Song &song : songs) {
          app_->queue()->Append(song);
        }
        RefreshQueue();
        break;
      }
      case SmartPlaylistsAction::QueueNext: {
        const SongList songs = item.search.Search(app_->collection()->Songs());
        for (auto it = songs.rbegin(); it != songs.rend(); ++it) {
          app_->queue()->InsertNext(*it);
        }
        RefreshQueue();
        break;
      }
      case SmartPlaylistsAction::Edit:
        if (item.kind == SmartPlaylistsItem::Kind::Saved) {
          Dialogs::SmartPlaylistWizard(GTK_WINDOW(window_), app_, item.title, item.search);
          smart_container_->Reload();
          RefreshPlaylistsList();
        }
        break;
      case SmartPlaylistsAction::Delete:
        if (item.kind == SmartPlaylistsItem::Kind::Saved) {
          SmartPlaylistSearch::RemoveSaved(item.title);
          smart_container_->Reload();
        }
        break;
      case SmartPlaylistsAction::New:
        Dialogs::SmartPlaylistWizard(GTK_WINDOW(window_), app_);
        smart_container_->Reload();
        RefreshPlaylistsList();
        RefreshPlaylist();
        break;
      case SmartPlaylistsAction::RestoreDefaults: {
        AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(
            Translations::CStr("Restore defaults"),
            Translations::CStr("Are you sure you want to restore the default smart playlists? This will remove all custom smart playlists")));
        adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "restore", Translations::CStr("Restore"), nullptr);
        adw_alert_dialog_set_response_appearance(dialog, "restore", ADW_RESPONSE_DESTRUCTIVE);
        g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *response, gpointer data) {
                           if (g_strcmp0(response, "restore") != 0) {
                             return;
                           }
                           auto *self = static_cast<MainWindow *>(data);
                           if (self->smart_container_) {
                             self->smart_container_->model()->RestoreDefaults();
                             self->smart_container_->Reload();
                           }
                         }),
                         this);
        adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
        break;
      }
    }
  });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, smart_container_->widget(), "smart", "Smart playlists", "view-refresh-symbolic");
  file_view_ = std::make_unique<FileView>();
  auto file_playlist = [this](FileViewMenu::Action action, const std::vector<std::string> &paths) {
    Settings settings;
    settings.BeginGroup(BehaviourSettings::kSettingsGroup);
    const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
        settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
    ApplyCollectionPlan(FileViewMenu::PlanFor(action, play, EngineStopped()), SongsFromFilePaths(paths));
  };
  file_view_->SetAddToPlaylistCallback([file_playlist](const std::vector<std::string> &paths) {
    file_playlist(FileViewMenu::Action::Append, paths);
  });
  file_view_->SetEnqueueCallback([this](const std::vector<std::string> &paths) {
    ApplyCollectionPlan(CollectionBehaviour::Enqueue(), SongsFromFilePaths(paths));
  });
  file_view_->SetReplacePlaylistCallback([file_playlist](const std::vector<std::string> &paths) {
    file_playlist(FileViewMenu::Action::Replace, paths);
  });
  file_view_->SetOpenInNewCallback([file_playlist](const std::vector<std::string> &paths) {
    file_playlist(FileViewMenu::Action::New, paths);
  });
  file_view_->SetCopyToCollectionCallback([this](const std::vector<std::string> &paths) { CopyFileViewToCollection(paths, false); });
  file_view_->SetMoveToCollectionCallback([this](const std::vector<std::string> &paths) { CopyFileViewToCollection(paths, true); });
  file_view_->SetCopyToDeviceCallback([this](const std::vector<std::string> &paths) {
    Dialogs::CopyToDevice(GTK_WINDOW(window_), app_, OrganizeLoading::FileViewFilenames(paths));
  });
  file_view_->SetEditTagsCallback([this](const std::vector<std::string> &paths) {
    const SongList songs = EditTagLoading::PlaceholdersFromPaths(paths);
    if (!EditTagLoading::OpensDialog(songs)) {
      ShowToast("No files selected");
      return;
    }
    Dialogs::EditTag(GTK_WINDOW(window_), app_, songs);
  });
  file_view_->SetDeleteCallback([this](const std::vector<std::string> &paths) {
    const SongList songs = FileViewSongs::FromPaths(paths);
    Dialogs::DeleteFiles(GTK_WINDOW(window_), app_, songs, DeleteFilesPolicy::SourceForSongs(songs));
    if (file_view_) {
      g_timeout_add(800, [](gpointer data) -> gboolean {
        static_cast<FileView *>(data)->Reload();
        return G_SOURCE_REMOVE;
      }, file_view_.get());
    }
  });
  file_view_->SetShowInBrowserCallback([this](const std::vector<std::string> &paths) {
    const std::string path = FileViewMenu::BrowserPath(paths);
    if (path.empty() || !FileManagerUtils::OpenInFileManager(path)) {
      ShowToast("Could not open the file manager");
    }
  });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, file_view_->widget(), "files", "Files", "folder-symbolic");
  radio_container_ = std::make_unique<RadioViewContainer>(app_->radio_services());
  radio_container_->SetActivateCallback([this](const RadioChannel &channel) { PlayRadioChannel(channel); });
  radio_container_->SetEnqueueCallback([this](const SongList &songs) { ApplyCollectionPlan(CollectionBehaviour::Enqueue(), songs); });
  radio_container_->SetMenuCallback([this](const std::vector<RadioChannel> &channels) { ShowRadioMenu(channels); });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, radio_container_->widget(), "radio", "Internet radio", "network-wireless-symbolic");
  GtkWidget *streaming_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  streaming_service_drop_ = gtk_combo_box_text_new();
  gtk_widget_set_margin_start(streaming_service_drop_, 8);
  gtk_widget_set_margin_end(streaming_service_drop_, 8);
  gtk_widget_set_margin_top(streaming_service_drop_, 6);
  gtk_widget_set_margin_bottom(streaming_service_drop_, 4);
  streaming_stack_ = gtk_stack_new();
  gtk_widget_set_vexpand(streaming_stack_, TRUE);
  auto activate_stream = [this](const Song &song) {
    app_->playlist_manager()->AppendSongs({song});
    RefreshPlaylist();
  };
  for (StreamingService *service : app_->streaming_services()->All()) {
    auto view = std::make_unique<StreamingTabsView>(service, app_->database());
    view->SetActivateCallback(activate_stream);
    view->SetEnqueueCallback([this](const SongList &songs) { ApplyCollectionPlan(CollectionBehaviour::Enqueue(), songs); });
    view->SetMenuCallback([this](const SongList &songs, StreamingCollectionActions::MenuContext ctx) { ShowStreamingMenu(songs, ctx); });
    const char *page = SettingsPages::ForService(service->name());
    view->SetConfigureCallback([this, page]() { OpenSettings(page); });
    view->SetFilterMenuCallback([this, page](CollectionFilterMenu::ActionKind kind) {
      if (kind == CollectionFilterMenu::ActionKind::Manage) {
        Dialogs::ManageSavedGroupings(GTK_WINDOW(window_), [this](const CollectionGrouping::Grouping &grouping) {
          grouping_ = grouping;
          CollectionGrouping::SaveCurrent(grouping);
          for (const auto &tabs : streaming_views_) {
            if (tabs && tabs->artists_collection_view()) {
              tabs->artists_collection_view()->ApplyGrouping(grouping);
            }
          }
        });
      } else if (kind == CollectionFilterMenu::ActionKind::Configure) {
        OpenSettings(page);
      }
    });
    gtk_stack_add_titled(GTK_STACK(streaming_stack_), view->widget(), service->name().c_str(), service->name().c_str());
    streaming_views_.push_back(std::move(view));
  }
  gtk_box_append(GTK_BOX(streaming_page), streaming_service_drop_);
  gtk_box_append(GTK_BOX(streaming_page), streaming_stack_);
  if (streaming_views_.empty()) {
    gtk_box_append(GTK_BOX(streaming_page), MakeScrolledList(&streaming_list_));
  }
  g_signal_connect(streaming_service_drop_, "changed", G_CALLBACK(+[](GtkComboBox *combo, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *id = gtk_combo_box_get_active_id(combo);
                     if (!id) {
                       return;
                     }
                     self->streaming_service_name_ = id;
                     gtk_stack_set_visible_child_name(GTK_STACK(self->streaming_stack_), id);
                   }),
                   this);
  adw_view_stack_add_titled_with_icon(sidebar_stack_, streaming_page, "streaming", "Streaming", "emblem-shared-symbolic");
  device_container_ = std::make_unique<DeviceViewContainer>(app_);
  device_container_->SetSongCallback([this](const Song &song) {
    app_->playlist_manager()->AppendSongs({song});
    RefreshPlaylist();
  });
  device_container_->SetActivateSongsCallback([this](const SongList &songs) {
    ApplyCollectionPlan(CollectionBehaviour::Append(BehaviourSettings::kDefaultMenuPlayMode, EngineStopped()), songs);
  });
  device_container_->SetEnqueueCallback([this](const SongList &songs) {
    ApplyCollectionPlan(CollectionBehaviour::Enqueue(), songs);
  });
  device_container_->SetAddAllCallback([this](const SongList &songs) {
    ApplyCollectionPlan(CollectionBehaviour::Append(BehaviourSettings::kDefaultMenuPlayMode, EngineStopped()), songs);
  });
  device_container_->SetPlaylistCallback([this](DeviceSongMenu::Action action, const SongList &songs) {
    Settings settings;
    settings.BeginGroup(BehaviourSettings::kSettingsGroup);
    const auto play = static_cast<BehaviourSettings::PlayBehaviour>(
        settings.IntValue(BehaviourSettings::kMenuPlayMode, static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode)));
    ApplyCollectionPlan(DeviceSongMenu::PlanFor(action, play, EngineStopped()), songs);
  });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, device_container_->widget(), "devices", "Devices", "drive-harddisk-usb-symbolic");
  queue_view_ = std::make_unique<QueueView>(app_->queue());
  queue_view_->SetActivateCallback([this](const Song &song) {
    app_->playlist_manager()->AppendSongs({song});
    app_->player()->PlayAt(app_->playlist_manager()->active()->row_count() - 1);
  });
  queue_view_->SetUrlDropCallback([this](const std::vector<std::string> &urls, int dest) {
    app_->queue()->Insert(dest, SongsFromUrls(urls));
    RefreshQueue();
  });
  queue_view_->SetPlaylistRowsDropCallback([this](const std::vector<int> &rows, int dest) {
    SongList songs;
    std::vector<QueueRows::Source> sources;
    Playlist *playlist = app_->playlist_manager()->current();
    if (!playlist) {
      return;
    }
    for (int row : rows) {
      if (row >= 0 && row < playlist->row_count()) {
        songs.push_back(playlist->song(row));
        sources.push_back({playlist->id(), row});
      }
    }
    app_->queue()->Insert(dest, songs, sources);
    RefreshQueue();
    RefreshPlaylist();
  });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, queue_view_->widget(), "queue", "Queue", "view-list-ordered-symbolic");

  if (streaming_list_) {
  g_signal_connect(streaming_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *kind = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-kind"));
                     if (kind && std::string(kind) == "back") {
                       self->streaming_service_name_.clear();
                       self->RefreshStreaming();
                       return;
                     }
                     if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"))) {
                       self->app_->playlist_manager()->AppendSongs({*song});
                       self->RefreshPlaylist();
                       return;
                     }
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-text"));
                     if (name) {
                       std::string label = name;
                       const auto signed_in = label.find(" (");
                       if (signed_in != std::string::npos) {
                         label = label.substr(0, signed_in);
                       }
                       if (self->app_->streaming_services()->ServiceByName(label)) {
                         self->streaming_service_name_ = label;
                         self->RefreshStreaming();
                       }
                     }
                   }),
                   this);
  }
}

void MainWindow::BuildContext() {
  context_view_ = std::make_unique<ContextView>(app_->lyrics_providers(), app_->lyrics_fetcher());
  context_view_->ReloadSettings();
  context_view_->SetSaveLyricsCallback([this](const std::string &lyrics) {
    Song song = app_->player()->current_song();
    song.set_lyrics(lyrics);
    if (app_->tagreader()->WriteFile(song) && song.id() > 0) {
      app_->collection()->backend()->AddOrUpdateSong(song);
    }
  });
  if (!cover_controller_) {
    cover_controller_ = std::make_unique<AlbumCoverChoiceController>(app_);
  }
  context_view_->SetCoverDropCallback([this](const std::vector<unsigned char> &data) {
    const Song song = app_->player()->current_song();
    CoverProviders::SaveAlbumCover(song, std::string(data.begin(), data.end()), app_->tagreader());
    context_view_->AlbumCoverLoaded(data);
  });
  context_view_->album_widget()->SetSearchCallback([this]() {
    if (cover_controller_) {
      Song song = app_->player()->current_song();
      cover_controller_->SearchCoverAutomatically(&song, context_view_->album_widget()->image());
      cover_controller_->SearchForCover(GTK_WINDOW(window_));
    }
  });
  cover_controller_->AttachMenu(context_view_->album_widget()->widget(), GTK_WINDOW(window_),
                                [this]() { return app_->player()->current_song(); });
  context_view_->album_widget()->SetActivateCallback([this]() {
    if (cover_controller_) {
      cover_controller_->ShowCover(GTK_WINDOW(window_), app_->player()->current_song());
    }
  });
}

void MainWindow::BuildPlaylist() {
  playlist_container_ = std::make_unique<PlaylistContainer>();
  playlist_container_->SetActionCallback("new", [this] { NewPlaylist(); });
  playlist_container_->SetActionCallback("load", [this] { LoadPlaylistFile(); });
  playlist_container_->SetActionCallback("save", [this] { SavePlaylistFile(); });
  playlist_container_->SetActionCallback("clear", [this] { ClearPlaylist(); });
  playlist_container_->SetActionCallback("undo", [this] { UndoPlaylist(); });
  playlist_container_->SetActionCallback("redo", [this] { RedoPlaylist(); });
  playlist_container_->SetFilterChangedCallback([this](const std::string &filter) {
    playlist_filter_ = filter;
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      playlist->SetFilterString(filter);
    }
    RefreshPlaylist();
  });
  playlist_container_->SetFilterRowCountCallback([this]() {
    Playlist *playlist = app_->playlist_manager() ? app_->playlist_manager()->current() : nullptr;
    return playlist ? playlist->row_count() : 0;
  });
  playlist_container_->view()->SetActivateCallback([this](int index) { app_->player()->PlayAt(index); });
  playlist_container_->view()->SetPlayPauseCallback([this]() { app_->player()->PlayPause(); });
  playlist_container_->view()->SetSeekBackwardCallback([this]() { app_->player()->SeekBackward(); });
  playlist_container_->view()->SetSeekForwardCallback([this]() { app_->player()->SeekForward(); });
  playlist_container_->view()->SetSelectCallback([this](int index, bool add) { SelectPlaylistRow(index, add); });
  playlist_container_->view()->SetRateCallback([this](int row, float rating) { RateRow(row, rating); });
  playlist_container_->view()->SetQueuePositionCallback([this](int row) {
    Playlist *playlist = app_->playlist_manager()->current();
    if (!playlist) {
      return 0;
    }
    return app_->queue()->PositionForPlaylistRow(playlist->id(), row);
  });
  playlist_container_->view()->SetSortCallback([this](PlaylistColumn column, PlaylistSortOrder order) { SortPlaylistBy(column, order); });
  playlist_container_->view()->SetMenuCallback([this](double x, double y) { ShowPlaylistMenu(x, y); });
  playlist_container_->view()->SetEditRequestCallback([this]() { EditColumnValue(); });
  playlist_container_->view()->SetDeleteCallback([this]() {
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      const std::vector<int> rows = SelectedPlaylistRows();
      app_->queue()->RemapAfterPlaylistRemove(playlist->id(), rows);
      playlist->RemoveRows(rows);
      selected_playlist_rows_.clear();
      app_->playlist_manager()->SaveCurrent();
      RefreshPlaylist();
      RefreshQueue();
    }
  });
  playlist_container_->view()->SetEditCommitCallback([this](int row, PlaylistColumn column, const std::string &value) {
    ApplyColumnValue(column, value, {row});
  });
  playlist_container_->dynamic_controls()->SetExpandCallback([this]() {
    app_->playlist_manager()->ExpandDynamic();
    RefreshPlaylist();
  });
  playlist_container_->dynamic_controls()->SetRepopulateCallback([this]() {
    app_->playlist_manager()->RepopulateDynamic();
    RefreshPlaylist();
  });
  playlist_container_->dynamic_controls()->SetTurnOffCallback([this]() {
    app_->playlist_manager()->TurnOffDynamic();
    RefreshPlaylist();
  });
  playlist_container_->view()->SetDropUrlsCallback([this](const std::vector<std::string> &urls, int row) {
    app_->playlist_manager()->InsertUrls(urls, row);
    RefreshPlaylist();
  });
  playlist_container_->view()->SetReorderCallback([this](const std::vector<int> &rows, int dest) {
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      app_->queue()->RemapAfterPlaylistMove(playlist->id(), playlist->row_count(), rows, dest);
      playlist->MoveRows(rows, dest);
      app_->playlist_manager()->SaveCurrent();
      RefreshPlaylist();
      RefreshQueue();
    }
  });
  playlist_container_->view()->SetCrossDropCallback([this](int source_id, const std::vector<int> &rows, int dest) {
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      app_->playlist_manager()->MoveRowsBetween(source_id, playlist->id(), rows, dest);
      RefreshPlaylist();
      RefreshPlaylistsList();
      RefreshQueue();
    }
  });
  playlist_container_->tab_bar()->SetChangedCallback([this](const std::string &name) {
    app_->playlist_manager()->SetCurrentPlaylist(name);
    RefreshPlaylist();
    RefreshPlaylistTabs();
  });
  playlist_container_->tab_bar()->SetFavoriteCallback([this](const std::string &name, bool on) {
    for (const auto &playlist : app_->playlist_manager()->playlists()) {
      if (playlist->name() == name) {
        app_->playlist_manager()->Favorite(playlist->id(), on);
        break;
      }
    }
  });
  playlist_container_->tab_bar()->SetCloseCallback([this](int id) { TryClosePlaylist(id); });
  playlist_container_->tab_bar()->SetRenameCallback([this](int id, const std::string &name) {
    if (name.empty()) {
      RenamePlaylist(id);
      return;
    }
    app_->playlist_manager()->Rename(id, name);
    RefreshPlaylistsList();
    RefreshPlaylist();
  });
  playlist_container_->tab_bar()->SetSaveCallback([this](int id) { SavePlaylistById(id); });
  playlist_container_->tab_bar()->SetNewCallback([this] { NewPlaylist(); });
  playlist_container_->tab_bar()->SetLoadCallback([this] { LoadPlaylistFile(); });
  playlist_container_->tab_bar()->SetReorderCallback([this](const std::vector<int> &ids) {
    app_->playlist_manager()->ChangePlaylistOrder(ids);
    RefreshPlaylistsList();
    RefreshPlaylistTabs();
  });
  playlist_container_->tab_bar()->SetLastTabCloseCallback([this] { HideToTray(); });
  playlist_container_->tab_bar()->SetDropCallback([this](int id, const std::string &payload) {
    if (id >= 0 && PlaylistListDrop::IsPlaylistRows(payload)) {
      const PlaylistDragPayload::Payload drag = PlaylistDragPayload::Decode(payload);
      const int source_id = drag.source_id >= 0 ? drag.source_id : app_->playlist_manager()->current_id();
      if (PlaylistListLook::ShouldAcceptPlaylistRowsDrop(id, source_id)) {
        app_->playlist_manager()->MoveRowsBetween(source_id, id, drag.rows);
      }
      RefreshPlaylist();
      RefreshPlaylistTabs();
      RefreshPlaylistsList();
      return;
    }
    if (id >= 0) {
      app_->playlist_manager()->SetCurrentPlaylist(id);
    }
    const std::vector<std::string> urls = PlaylistListDrop::ParseUrls(payload);
    if (!urls.empty()) {
      app_->playlist_manager()->InsertUrls(urls, -1);
    }
    RefreshPlaylist();
    RefreshPlaylistTabs();
  });

  repeat_button_ = playlist_container_->repeat_button();
  shuffle_button_ = playlist_container_->shuffle_button();
  playlist_container_->SetRepeatChangedCallback([this](PlaylistSequence::RepeatMode mode) {
    playlist_sequence_.SetRepeatMode(mode);
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      playlist->SetRepeatMode(mode);
    }
  });
  playlist_container_->SetShuffleChangedCallback([this](PlaylistSequence::ShuffleMode mode) {
    playlist_sequence_.SetShuffleMode(mode);
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      playlist->SetShuffleMode(mode);
    }
  });
  playlist_container_->SetRepeatMode(playlist_sequence_.repeat_mode());
  playlist_container_->SetShuffleMode(playlist_sequence_.shuffle_mode());
}

void MainWindow::BuildPlayerBar() {
  playing_widget_ = std::make_unique<PlayingWidget>();
  if (!cover_controller_) {
    cover_controller_ = std::make_unique<AlbumCoverChoiceController>(app_);
  }
  cover_controller_->AttachMenu(playing_widget_->cover(), GTK_WINDOW(window_), [this]() { return app_->player()->current_song(); });
  cover_controller_->SetSearchAutoChangedCallback([this](bool) {
    if (context_view_) {
      context_view_->ReloadSettings();
    }
  });
  playing_widget_->SetSearchAutoChangedCallback([this](bool) {
    if (context_view_) {
      context_view_->ReloadSettings();
    }
  });
  playing_widget_->SetCoverActionCallback([this](CoverChoiceMenu::Action action) {
    Song song = app_->player()->current_song();
    cover_controller_->Perform(action, GTK_WINDOW(window_), &song, playing_widget_->cover());
  });
  playing_widget_->SetDropCallback([this](const std::vector<unsigned char> &data) {
    const Song song = app_->player()->current_song();
    CoverProviders::SaveAlbumCover(song, std::string(data.begin(), data.end()), app_->tagreader());
    playing_widget_->SetCover(data);
    if (context_view_) {
      context_view_->AlbumCoverLoaded(data);
    }
  });
  GtkGesture *cover_activate = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(cover_activate), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(playing_widget_->cover(), GTK_EVENT_CONTROLLER(cover_activate));
  g_signal_connect(cover_activate, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint n_press, gdouble, gdouble, gpointer data) {
                     if (n_press != 2) {
                       return;
                     }
                     auto *self = static_cast<MainWindow *>(data);
                     if (self->cover_controller_) {
                       self->cover_controller_->ShowCover(GTK_WINDOW(self->window_), self->app_->player()->current_song());
                     }
                   }),
                   this);
  track_slider_ = std::make_unique<TrackSlider>();
  volume_slider_ = std::make_unique<VolumeSlider>();
  track_slider_->SetSeekCallback([this](int64_t pos) { app_->player()->Seek(pos); });
  track_slider_->SetSeekStepCallbacks([this]() { app_->player()->SeekBackward(); }, [this]() { app_->player()->SeekForward(); });
  volume_slider_->SetVolumeCallback([this](unsigned volume) { app_->player()->SetVolume(volume); });
  volume_slider_->SetVolume(app_->player()->GetVolume());

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  gtk_box_append(GTK_BOX(box), playing_widget_->widget());
  playing_widget_->AboveStatusBarChanged.Connect([this](bool) { PlacePlayingWidget(); });

  waveform_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(waveform_drawing_, -1, 28);
  gtk_widget_set_hexpand(waveform_drawing_, TRUE);
  gtk_widget_set_halign(waveform_drawing_, GTK_ALIGN_FILL);
  gtk_widget_set_valign(waveform_drawing_, GTK_ALIGN_FILL);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(waveform_drawing_), DrawWaveform, this, nullptr);
  moodbar_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(moodbar_drawing_, -1, 16);
  gtk_widget_set_hexpand(moodbar_drawing_, TRUE);
  gtk_widget_set_halign(moodbar_drawing_, GTK_ALIGN_FILL);
  gtk_widget_set_valign(moodbar_drawing_, GTK_ALIGN_FILL);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(moodbar_drawing_), DrawMoodbar, this, nullptr);
  gtk_overlay_add_overlay(GTK_OVERLAY(track_slider_->slider_overlay()), waveform_drawing_);
  gtk_overlay_add_overlay(GTK_OVERLAY(track_slider_->slider_overlay()), moodbar_drawing_);
  gtk_box_append(GTK_BOX(box), track_slider_->widget());
  auto attach_seek = [&](GtkWidget *widget, bool hover_and_wheel) {
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint, gdouble x, gdouble, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       GtkWidget *area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
                       self->SeekFromBar(x, gtk_widget_get_width(area));
                     }),
                     this);
    GtkGesture *menu = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(menu));
    g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint, gdouble, gdouble, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       self->ShowSeekbarMenu(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
                     }),
                     this);
    if (!hover_and_wheel) {
      return;
    }
    GtkEventController *scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    gtk_widget_add_controller(widget, scroll);
    g_signal_connect(scroll, "scroll", G_CALLBACK((+[](GtkEventControllerScroll *, gdouble, gdouble dy, gpointer data) -> gboolean {
                       static_cast<MainWindow *>(data)->OnSeekbarScroll(dy);
                       return TRUE;
                     })),
                     this);
    GtkEventController *motion = gtk_event_controller_motion_new();
    gtk_widget_add_controller(widget, motion);
    g_signal_connect(motion, "motion", G_CALLBACK(+[](GtkEventControllerMotion *controller, gdouble x, gdouble, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       GtkWidget *area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
                       if (self->track_slider_) {
                         self->track_slider_->ShowHoverAt(area, x, gtk_widget_get_width(area));
                       }
                     }),
                     this);
    g_signal_connect(motion, "leave", G_CALLBACK(+[](GtkEventControllerMotion *, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       if (self->track_slider_) {
                         self->track_slider_->HideHover();
                       }
                     }),
                     this);
  };
  attach_seek(moodbar_drawing_, true);
  attach_seek(waveform_drawing_, true);
  attach_seek(track_slider_->slider()->widget(), false);
  GtkGesture *slider_box_menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(slider_box_menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(track_slider_->widget(), GTK_EVENT_CONTROLLER(slider_box_menu));
  g_signal_connect(slider_box_menu, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     self->ShowSeekbarMenu(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
                   }),
                   this);
  ApplySeekbarMode();

  GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_add_css_class(controls, "strawberry-play-controls");
  gtk_widget_set_halign(controls, GTK_ALIGN_CENTER);
  GtkWidget *prev = gtk_button_new_from_icon_name("media-skip-backward-symbolic");
  play_button_ = gtk_button_new_from_icon_name("media-playback-start-symbolic");
  gtk_widget_add_css_class(play_button_, "suggested-action");
  gtk_widget_add_css_class(play_button_, "circular");
  GtkWidget *stop = adw_split_button_new();
  adw_split_button_set_icon_name(ADW_SPLIT_BUTTON(stop), "media-playback-stop-symbolic");
  gtk_widget_set_tooltip_text(stop, Translations::CStr(MainWindowMenu::Stop()));
  GMenu *stop_menu = g_menu_new();
  g_menu_append(stop_menu, Translations::CStr(MainWindowMenu::Stop()), "win.stop");
  g_menu_append(stop_menu, Translations::CStr(MainWindowMenu::StopAfter()), "win.stop-after");
  adw_split_button_set_menu_model(ADW_SPLIT_BUTTON(stop), G_MENU_MODEL(stop_menu));
  GtkWidget *next = gtk_button_new_from_icon_name("media-skip-forward-symbolic");
  scrobble_button_ = gtk_button_new_from_icon_name("document-send-symbolic");
  gtk_widget_set_tooltip_text(scrobble_button_, "Scrobble current track");
  love_button_ = gtk_button_new_from_icon_name("emblem-favorite-symbolic");
  gtk_widget_set_tooltip_text(love_button_, Translations::CStr(MainWindowMenu::Love()));
  analyzer_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(analyzer_drawing_, 160, 36);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(analyzer_drawing_), DrawAnalyzer, this, nullptr);
  gtk_box_append(GTK_BOX(controls), prev);
  gtk_box_append(GTK_BOX(controls), play_button_);
  gtk_box_append(GTK_BOX(controls), stop);
  gtk_box_append(GTK_BOX(controls), next);
  gtk_box_append(GTK_BOX(controls), scrobble_button_);
  gtk_box_append(GTK_BOX(controls), love_button_);
  GtkGesture *analyzer_menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(analyzer_menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(analyzer_drawing_, GTK_EVENT_CONTROLLER(analyzer_menu));
  g_signal_connect(analyzer_menu, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble, gdouble, gpointer data) {
                     static_cast<MainWindow *>(data)->ShowAnalyzerMenu();
                   }),
                   this);
  gtk_box_append(GTK_BOX(controls), analyzer_drawing_);
  GtkEventController *analyzer_scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(analyzer_drawing_, analyzer_scroll);
  g_signal_connect(analyzer_scroll, "scroll", G_CALLBACK((+[](GtkEventControllerScroll *, gdouble, gdouble dy, gpointer data) -> gboolean {
                     auto *self = static_cast<MainWindow *>(data);
                     if (self->volume_slider_) {
                       self->volume_slider_->HandleGtkScroll(dy);
                     }
                     return TRUE;
                   })),
                   this);
  ApplyAnalyzer();
  mute_button_ = gtk_button_new_from_icon_name(MainWindowLook::MuteIconName(false));
  gtk_widget_set_tooltip_text(mute_button_, Translations::CStr(MainWindowLook::MuteTooltip(false)));
  gtk_widget_add_css_class(mute_button_, "flat");
  g_signal_connect(mute_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->ToggleMute(); }), this);
  gtk_box_append(GTK_BOX(controls), mute_button_);
  gtk_box_append(GTK_BOX(controls), volume_slider_->widget());
  ApplyMuteUi(app_->player()->GetVolume());
  gtk_box_append(GTK_BOX(box), controls);

  loading_indicator_ = std::make_unique<MultiLoadingIndicator>();
  status_label_ = gtk_label_new("");
  gtk_widget_add_css_class(status_label_, "dim-label");
  gtk_widget_set_hexpand(status_label_, TRUE);
  gtk_widget_set_halign(status_label_, GTK_ALIGN_END);
  gtk_label_set_xalign(GTK_LABEL(status_label_), 1.0f);
  gtk_label_set_ellipsize(GTK_LABEL(status_label_), PANGO_ELLIPSIZE_END);
  status_bar_stack_ = gtk_stack_new();
  gtk_widget_set_hexpand(status_bar_stack_, TRUE);
  gtk_stack_set_hhomogeneous(GTK_STACK(status_bar_stack_), TRUE);
  gtk_stack_add_named(GTK_STACK(status_bar_stack_), loading_indicator_->widget(),
                      StatusBarStack::ChildName(StatusBarStack::Page::Loading));
  gtk_stack_add_named(GTK_STACK(status_bar_stack_), status_label_, StatusBarStack::ChildName(StatusBarStack::Page::Summary));
  gtk_stack_set_visible_child_name(GTK_STACK(status_bar_stack_), StatusBarStack::ChildName(StatusBarStack::Page::Summary));
  gtk_box_append(GTK_BOX(box), status_bar_stack_);

  g_signal_connect(play_button_, "clicked", G_CALLBACK(OnPlayPause), this);
  g_signal_connect(stop, "clicked", G_CALLBACK(+[](AdwSplitButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Stop(); }),
                   this);
  g_signal_connect(next, "clicked", G_CALLBACK(OnNext), this);
  g_signal_connect(prev, "clicked", G_CALLBACK(OnPrevious), this);
  g_signal_connect(scrobble_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->ScrobbleCurrent(); }),
                   this);
  g_signal_connect(love_button_, "clicked", G_CALLBACK(OnLove), this);
  UpdateScrobblerButtons();
  g_object_set_data(G_OBJECT(play_button_), "player-box", box);
  PlacePlayingWidget();
}

void MainWindow::PlacePlayingWidget() {
  if (!playing_widget_ || !play_button_) {
    return;
  }
  GtkWidget *box = GTK_WIDGET(g_object_get_data(G_OBJECT(play_button_), "player-box"));
  if (!box) {
    return;
  }
  GtkWidget *playing = playing_widget_->widget();
  if (playing_widget_->above_status_bar() && status_bar_stack_) {
    gtk_box_reorder_child_after(GTK_BOX(box), playing, status_bar_stack_);
    gtk_box_reorder_child_after(GTK_BOX(box), status_bar_stack_, playing);
  } else {
    gtk_box_reorder_child_after(GTK_BOX(box), playing, nullptr);
  }
}

void MainWindow::ConnectSignals() {
  app_->RaiseRequested.Connect([this]() { Present(); });
  app_->playlist_manager()->SequenceChanged.Connect([this]() {
    Playlist *playlist = app_->playlist_manager()->current();
    if (!playlist) {
      return;
    }
    playlist_sequence_.SetRepeatMode(playlist->repeat_mode());
    playlist_sequence_.SetShuffleMode(playlist->shuffle_mode());
    if (playlist_container_) {
      playlist_container_->SetRepeatMode(playlist_sequence_.repeat_mode());
      playlist_container_->SetShuffleMode(playlist_sequence_.shuffle_mode());
    }
    RefreshPlaylist();
  });
  app_->player()->SongChanged.Connect([this](const Song &) {
    UpdateNowPlaying();
    SelectPlayingTrack();
    ApplySeekbarPlaybackState();
  });
  app_->player()->Playing.Connect([this]() {
    ApplySeekbarPlaybackState();
    if (playlist_list_container_) {
      playlist_list_container_->SetPlayback(PlaylistListLook::Playback::Playing);
    }
    if (playlist_container_) {
      playlist_container_->view()->SetPaused(false);
      playlist_container_->view()->SetGlowing(true);
    }
    RefreshPlaylistTabs();
  });
  app_->player()->Paused.Connect([this]() {
    if (playlist_list_container_) {
      playlist_list_container_->SetPlayback(PlaylistListLook::Playback::Paused);
    }
    if (playlist_container_) {
      playlist_container_->view()->SetPaused(true);
      playlist_container_->view()->SetGlowing(false);
    }
    RefreshPlaylistTabs();
  });
  app_->player()->Stopped.Connect([this]() {
    ApplySeekbarPlaybackState();
    if (playing_widget_) {
      playing_widget_->Stopped();
    }
    if (context_view_) {
      context_view_->Stopped();
    }
    if (playlist_list_container_) {
      playlist_list_container_->SetPlayback(PlaylistListLook::Playback::Stopped);
    }
    if (playlist_container_) {
      playlist_container_->view()->SetPaused(false);
      playlist_container_->view()->SetGlowing(false);
    }
    UpdatePlaybackButtons();
    RefreshPlaylistTabs();
  });
  app_->playlist_manager()->ActiveChanged.Connect([this](Playlist *playlist) {
    if (playlist_list_container_) {
      playlist_list_container_->SetActive(playlist ? playlist->name() : "", playlist ? playlist->id() : -1);
    }
    RefreshPlaylistTabs();
  });
  app_->playlist_manager()->CurrentChanged.Connect([this](Playlist *playlist) {
    if (playlist_list_container_) {
      playlist_list_container_->Reload(app_->playlist_manager());
      if (playlist) {
        playlist_list_container_->SelectName(playlist->name());
      }
    }
    if (queue_view_) {
      queue_view_->SetQueue(app_->queue());
    }
    app_->queue()->Changed.Connect([this]() {
      RefreshQueue();
      if (PlaylistQueueRefresh::ShouldRefreshPlaylistView()) {
        RefreshPlaylist();
      }
    });
    RefreshQueue();
    RefreshPlaylistTabs();
  });
  app_->player()->StateChanged.Connect([this](GstEngine::State state) {
    if (playing_widget_) {
      if (state == GstEngine::State::Playing) {
        playing_widget_->Playing();
      } else if (state == GstEngine::State::Error) {
        playing_widget_->Error();
      }
    }
    UpdatePlaybackButtons();
  });
  app_->player()->VolumeChanged.Connect([this](unsigned volume) {
    if (volume_slider_) {
      volume_slider_->SetVolume(volume);
    }
    ApplyMuteUi(volume);
  });
  app_->playlist_manager()->PlaylistsLoaded.Connect([this]() {
    HandlePlaylistsLoaded();
    ApplyPlaylistBehaviour();
    RefreshPlaylistsList();
    RefreshPlaylist();
    if (playlist_container_ && playlist_container_->view()) {
      Playlist *playlist = app_->playlist_manager()->current();
      const int row = PlaylistRestoreScroll::TargetRow(playlist ? playlist->current_row() : -1, playlist ? playlist->current_row() : -1);
      if (PlaylistRestoreScroll::ShouldJump(row)) {
        playlist_container_->view()->JumpToCurrentlyPlayingTrack();
      }
    }
  });
  app_->scrobbler()->Error.Connect([this](const std::string &message) {
    if (ScrobblerError::ShouldShowDialog(message)) {
      ShowToast(message);
    }
  });
  app_->playlist_manager()->Error.Connect([this](const std::string &message) { ShowToast(message); });
  app_->scrobbler()->EnabledChanged.Connect([this](bool) { UpdateScrobblerButtons(); });
  app_->scrobbler()->TrackLoved.Connect([this](const Song &) {
    loved_current_track_ = ScrobblerLoveState::DisableAfterLove();
    UpdateScrobblerButtons();
  });
  app_->collection()->ScanFinished.Connect([this]() {
    RefreshCollection();
    ShowToast("Collection scan finished");
  });
  app_->collection()->backend()->SongsChanged.Connect([this](const SongList &songs) {
    ApplyCollectionIncremental(CollectionModelUpdateType::UpdateSongs, songs);
  });
  app_->collection()->backend()->SongsDiscovered.Connect([this](const SongList &songs) {
    ApplyCollectionIncremental(CollectionModelUpdateType::AddSongs, songs);
  });
  app_->collection()->backend()->SongsDeleted.Connect([this](const SongList &songs) {
    ApplyCollectionIncremental(CollectionModelUpdateType::RemoveSongs, songs);
  });
  auto patch_collection_songs = [this](const SongList &songs) {
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      for (const Song &song : songs) {
        playlist->PatchSongById(song);
      }
    }
    ApplyCollectionIncremental(CollectionModelUpdateType::UpdateSongs, songs);
    RefreshPlaylist();
  };
  app_->collection()->backend()->SongsStatisticsChanged.Connect(patch_collection_songs);
  app_->collection()->backend()->SongsRatingChanged.Connect(patch_collection_songs);
  app_->task_manager()->TasksChanged.Connect([this](int) {
    if (!loading_indicator_) {
      return;
    }
    const std::vector<TaskManager::Task> tasks = app_->task_manager()->GetTasks();
    std::vector<MultiLoadingText::Task> infos;
    infos.reserve(tasks.size());
    for (const auto &task : tasks) {
      infos.push_back({task.name, task.progress, task.progress_max});
    }
    loading_indicator_->SetTasks(infos);
    if (status_bar_stack_) {
      gtk_stack_set_visible_child_name(GTK_STACK(status_bar_stack_),
                                       StatusBarStack::ChildName(StatusBarStack::PageForTaskCount(static_cast<int>(tasks.size()))));
    }
  });
  app_->current_albumcover_loader()->AlbumCoverReady.Connect([this](const Song &, const std::vector<unsigned char> &data) {
    if (!data.empty()) {
      UpdateCover(data);
    }
  });
  app_->queue()->Changed.Connect([this]() {
    RefreshQueue();
    if (PlaylistQueueRefresh::ShouldRefreshPlaylistView()) {
      RefreshPlaylist();
    }
  });
  app_->device_manager()->DevicesChanged.Connect([this]() { RefreshDevices(); });
  app_->radio_services()->set_updated_callback([this]() { RefreshRadio(); });
  app_->waveform()->Ready.Connect([this](const std::vector<float> &) { gtk_widget_queue_draw(waveform_drawing_); });
  app_->moodbar()->Ready.Connect([this](const std::vector<uint8_t> &) { gtk_widget_queue_draw(moodbar_drawing_); });
  app_->player()->engine()->ScopeUpdated.Connect([this](const std::vector<int16_t> &scope) {
    if (!app_->analyzer()->enabled()) {
      return;
    }
    const auto state = app_->player()->GetState();
    app_->analyzer()->set_paused(state == GstEngine::State::Paused);
    app_->analyzer()->set_playing(state == GstEngine::State::Playing);
    app_->analyzer()->SetEngineScope(scope);
  });
  EnsureAnalyzerTimer();
  position_timeout_ = g_timeout_add(200, [](gpointer data) -> gboolean {
    auto *self = static_cast<MainWindow *>(data);
    if (!self->app_->player()->engine()) {
      return G_SOURCE_CONTINUE;
    }
    const int64_t pos = self->app_->player()->engine()->position_nanosec();
    const int64_t len = self->app_->player()->engine()->length_nanosec();
    if (self->track_slider_) {
      if (self->EngineStopped()) {
        self->track_slider_->SetStopped();
      } else {
        self->track_slider_->SetTimes(pos, len);
      }
    }
    if (len > 0) {
      self->app_->tray()->SetProgress(static_cast<int>(pos * 100 / len));
    }
    {
      Settings settings;
      settings.BeginGroup(BehaviourSettings::kSettingsGroup);
      const bool enabled = settings.BoolValue(BehaviourSettings::kTaskbarProgress, BehaviourSettings::kDefaultTaskbarProgress);
      const bool playing = self->app_->player()->GetState() == EngineBase::State::Playing;
      self->taskbar_.Set(TaskbarProgressHelpers::Fraction(pos, len), TaskbarProgressHelpers::ShouldShow(enabled, playing, len));
    }
    if (self->playlist_container_ && self->playlist_container_->view()) {
      self->playlist_container_->view()->SetPlaybackProgress(len > 0 ? static_cast<double>(pos) / static_cast<double>(len) : 0.0);
    }
    if (self->app_->mpris() && self->app_->player()->GetState() == EngineBase::State::Playing) {
      self->app_->mpris()->EmitPosition();
    }
    if (self->moodbar_drawing_ && gtk_widget_get_visible(self->moodbar_drawing_)) {
      gtk_widget_queue_draw(self->moodbar_drawing_);
    }
    if (self->waveform_drawing_ && gtk_widget_get_visible(self->waveform_drawing_)) {
      gtk_widget_queue_draw(self->waveform_drawing_);
    }
    if (self->context_view_) {
      self->context_view_->SetPlaybackPosition(pos);
    }
    if (self->app_->scrobbler() && self->app_->playlist_manager()) {
      Playlist *playlist = self->app_->playlist_manager()->active();
      const Song song = self->app_->player()->current_song();
      if (playlist &&
          ScrobblePoint::ShouldSubmit(self->app_->scrobbler()->enabled(), playlist->scrobbled(), song.is_metadata_good(), pos,
                                      playlist->scrobble_point_nanosec())) {
        self->app_->scrobbler()->Scrobble(song);
        playlist->set_scrobbled(true);
      }
    }
    return G_SOURCE_CONTINUE;
  }, this);
}

void MainWindow::ApplyCollectionIncremental(CollectionModelUpdateType type, const SongList &songs) {
  if (!CollectionIncremental::ShouldApply(app_->collection()->scanning())) {
    return;
  }
  if (!collection_container_ || !collection_container_->view()) {
    return;
  }
  collection_container_->view()->ApplyUpdate(CollectionIncremental::Make(type, songs));
  if (context_view_) {
    context_view_->SetCollectionTotals(collection_container_->view()->model()->TotalSongs(),
                                      collection_container_->view()->model()->TotalArtists(),
                                      collection_container_->view()->model()->TotalAlbums());
  }
}

void MainWindow::RefreshCollection(const std::string &filter, bool update_text) {
  if (update_text) {
    collection_text_filter_ = filter;
    if (collection_search_ && CollectionSearchSync::ShouldUpdateEntry(true) &&
        CollectionSearchSync::TextDiffers(gtk_editable_get_text(GTK_EDITABLE(collection_search_)), filter)) {
      gtk_editable_set_text(GTK_EDITABLE(collection_search_), filter.c_str());
    }
  }
  if (!collection_container_) {
    return;
  }
  CollectionFilterOptions options = collection_container_->filter_widget()->options();
  SongList songs = app_->collection()->Songs(options);
  Settings settings;
  settings.BeginGroup("Collection");
  collection_container_->view()->SetModelSongs(
      songs, grouping_, CollectionGrouping::SeparateAlbumsByGrouping(),
      settings.BoolValue("skip_articles_for_artists", settings.BoolValue("sort_skip_articles_for_artists", true)),
      settings.BoolValue("skip_articles_for_albums", settings.BoolValue("sort_skip_articles_for_albums", false)));
  collection_container_->view()->SetFilterString(collection_text_filter_);
  if (context_view_) {
    context_view_->SetCollectionTotals(collection_container_->view()->model()->TotalSongs(),
                                      collection_container_->view()->model()->TotalArtists(),
                                      collection_container_->view()->model()->TotalAlbums());
  }
}

void MainWindow::SortPlaylistBy(PlaylistColumn column, PlaylistSortOrder order) {
  if (!PlaylistHeaderSort::ShouldApplyExplicit(column, sort_column_, sort_descending_, order) && order != PlaylistSortOrder::Toggle) {
    return;
  }
  const PlaylistHeaderSort::State next = PlaylistHeaderSort::ApplyOrder({sort_column_, sort_descending_}, column, order);
  sort_column_ = next.column;
  sort_descending_ = next.descending;
  Playlist *playlist = app_->playlist_manager()->current();
  if (playlist) {
    ApplyPlaylistBehaviour();
    playlist->SetSort(sort_column_, sort_descending_);
    if (PlaylistHeaderSort::ShouldSortNow(sort_column_)) {
      playlist->SortNow();
      app_->playlist_manager()->SaveCurrent();
    }
  }
  RefreshPlaylist();
}

void MainWindow::SelectPlaylistRow(int index, bool add) {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist || index < 0 || index >= playlist->row_count()) {
    return;
  }
  if (!add) {
    selected_playlist_rows_.clear();
    selected_playlist_rows_.push_back(index);
  } else {
    auto it = std::find(selected_playlist_rows_.begin(), selected_playlist_rows_.end(), index);
    if (it == selected_playlist_rows_.end()) {
      selected_playlist_rows_.push_back(index);
      std::sort(selected_playlist_rows_.begin(), selected_playlist_rows_.end());
    } else {
      selected_playlist_rows_.erase(it);
    }
  }
  selection_playlist_name_ = playlist->name();
  app_->playlist_manager()->SetCurrentRow(index);
  if (playlist_container_) {
    playlist_container_->view()->SetSelectedRows(selected_playlist_rows_);
    playlist_container_->view()->Refresh(playlist);
  }
  RefreshPlaylistSummary();
}

void MainWindow::RefreshPlaylist() {
  if (!playlist_container_) {
    return;
  }
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist || playlist->name() != selection_playlist_name_) {
    selected_playlist_rows_.clear();
    selection_playlist_name_ = playlist ? playlist->name() : std::string();
  }
  playlist_container_->ApplyLook();
  if (playlist) {
    const std::string stored = PlaylistFilterSync::FilterForPlaylist(playlist);
    if (PlaylistFilterSync::ShouldSyncEntry(playlist_container_->filter_string(), stored)) {
      playlist_container_->SetFilterText(PlaylistFilterSync::EntryFromPlaylist(stored));
    }
    playlist_filter_ = PlaylistFilterSync::FilterForPlaylist(playlist);
    playlist_container_->UpdateUndoRedoChrome(playlist->CanUndo(), playlist->CanRedo());
  } else {
    playlist_filter_.clear();
    playlist_container_->SetFilterText({});
    playlist_container_->UpdateUndoRedoChrome(false, false);
  }
  playlist_container_->view()->SetFilterString(playlist_filter_);
  playlist_container_->view()->SetSelectedRows(selected_playlist_rows_);
  playlist_container_->view()->Refresh(playlist);
  playlist_container_->UpdateNoMatchesOverlay();
  if (playlist) {
    playlist_container_->dynamic_controls()->SetSearch(playlist->dynamic_search());
    playlist_container_->dynamic_controls()->SetVisible(playlist->is_dynamic());
  } else {
    playlist_container_->dynamic_controls()->SetVisible(false);
  }
  RefreshPlaylistSummary();
}

void MainWindow::RefreshPlaylistSummary() {
  std::string text;
  Playlist *playlist = app_->playlist_manager() ? app_->playlist_manager()->current() : nullptr;
  if (playlist) {
    text = PlaylistSummary::Format(
        PlaylistSummary::FromPlaylist(playlist->row_count(), playlist->total_length_nanosec(), playlist->songs(), selected_playlist_rows_));
  }
  if (status_label_) {
    gtk_label_set_text(GTK_LABEL(status_label_), text.c_str());
  }
  if (playlist_container_) {
    playlist_container_->SetSummary(text);
  }
}

void MainWindow::GoToPlaylistIndex(int index) {
  if (!app_ || !app_->playlist_manager() || index < 0) {
    return;
  }
  const auto &playlists = app_->playlist_manager()->playlists();
  if (index >= static_cast<int>(playlists.size())) {
    return;
  }
  app_->playlist_manager()->SetCurrentPlaylist(playlists[static_cast<size_t>(index)]->id());
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::NextPlaylistTab() {
  if (!app_ || !app_->playlist_manager()) {
    return;
  }
  const auto &playlists = app_->playlist_manager()->playlists();
  int current = -1;
  for (size_t i = 0; i < playlists.size(); ++i) {
    if (playlists[i].get() == app_->playlist_manager()->current()) {
      current = static_cast<int>(i);
      break;
    }
  }
  GoToPlaylistIndex(PlaylistTabNavigation::NextIndex(current, static_cast<int>(playlists.size())));
}

void MainWindow::PreviousPlaylistTab() {
  if (!app_ || !app_->playlist_manager()) {
    return;
  }
  const auto &playlists = app_->playlist_manager()->playlists();
  int current = -1;
  for (size_t i = 0; i < playlists.size(); ++i) {
    if (playlists[i].get() == app_->playlist_manager()->current()) {
      current = static_cast<int>(i);
      break;
    }
  }
  GoToPlaylistIndex(PlaylistTabNavigation::PreviousIndex(current, static_cast<int>(playlists.size())));
}

void MainWindow::LastPlaylistTab() {
  if (!app_ || !app_->playlist_manager()) {
    return;
  }
  GoToPlaylistIndex(PlaylistTabNavigation::LastIndex(static_cast<int>(app_->playlist_manager()->playlists().size())));
}

void MainWindow::ActivePlaylistTab() {
  if (!app_ || !app_->playlist_manager()) {
    return;
  }
  const auto &playlists = app_->playlist_manager()->playlists();
  int active = -1;
  for (size_t i = 0; i < playlists.size(); ++i) {
    if (playlists[i].get() == app_->playlist_manager()->active()) {
      active = static_cast<int>(i);
      break;
    }
  }
  GoToPlaylistIndex(PlaylistTabNavigation::ActiveIndex(active, static_cast<int>(playlists.size())));
}

void MainWindow::RefreshPlaylistsList() {
  if (playlist_list_container_) {
    playlist_list_container_->Reload(app_->playlist_manager());
  }
  RefreshPlaylistTabs();
}

void MainWindow::RefreshPlaylistTabs() {
  if (playlist_container_) {
    playlist_container_->tab_bar()->Refresh(app_->playlist_manager(),
                                            playlist_list_container_ ? playlist_list_container_->active_name() : std::string(),
                                            playlist_list_container_ ? playlist_list_container_->playback()
                                                                     : PlaylistListLook::Playback::Stopped);
  }
}

void MainWindow::RefreshSmartPlaylists() {
  if (smart_container_) {
    smart_container_->Reload();
  }
}

void MainWindow::RefreshQueue() {
  if (queue_view_) {
    queue_view_->SetNowPlayingUrl(app_->player()->current_song().url());
    queue_view_->Reload();
  }
}

void MainWindow::RefreshRadio() {
  if (radio_container_) {
    radio_container_->Reload();
  }
}

void MainWindow::SearchRadio(const std::string &query) {
  radio_query_ = StrUtils::Trim(query);
  if (radio_container_) {
    radio_container_->Search(radio_query_);
  }
}

void MainWindow::RefreshStreaming() {
  if (streaming_service_drop_) {
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(streaming_service_drop_));
    for (StreamingService *service : app_->streaming_services()->All()) {
      service->ReloadSettings();
      const std::string label = service->name() + (service->logged_in() ? " (signed in)" : "");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(streaming_service_drop_), service->name().c_str(), label.c_str());
    }
    if (!streaming_service_name_.empty()) {
      gtk_combo_box_set_active_id(GTK_COMBO_BOX(streaming_service_drop_), streaming_service_name_.c_str());
    } else if (!app_->streaming_services()->All().empty()) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(streaming_service_drop_), 0);
    }
  }
  for (const auto &view : streaming_views_) {
    view->ReloadSettings();
  }
  if (!streaming_list_ || !streaming_views_.empty()) {
    return;
  }
  ClearList(streaming_list_);
  if (app_->streaming_services()->All().empty()) {
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Subsonic — enable in Preferences", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Tidal — enable in Preferences", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Spotify — enable in Preferences", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Qobuz — enable in Preferences", nullptr);
  }
}

void MainWindow::SearchStreaming(const std::string &query) {
  if (query.empty()) {
    return;
  }
  for (const auto &view : streaming_views_) {
    if (view->search_view()) {
      view->search_view()->Search(query);
    }
  }
}

void MainWindow::RefreshDevices() {
  if (refreshing_devices_) {
    return;
  }
  refreshing_devices_ = true;
  if (device_container_) {
    device_container_->Reload();
  }
  refreshing_devices_ = false;
}

void MainWindow::RefreshFiles() {
  if (file_view_) {
    file_view_->Reload();
  }
}

void MainWindow::UpdateNowPlaying() {
  const Song now = app_->player()->current_song();
  if (ScrobblerLoveState::ResetLovedOnSongChange(loved_song_url_, now.url())) {
    loved_current_track_ = false;
    loved_song_url_ = now.url();
  }
  UpdateScrobblerButtons();
  const Song song = app_->player()->current_song();
  if (playing_widget_) {
    playing_widget_->SongChanged(song);
  }
  if (context_view_) {
    context_view_->SongChanged(song);
  }
  RefreshPlaylist();
  const auto embedded = app_->albumcover_loader()->LoadData(song);
  if (!embedded.empty()) {
    UpdateCover(embedded);
  } else {
    app_->cover_providers()->FetchFromEmbeddedOrFile(song, [this](const std::string &data, const std::string &) {
      if (!data.empty()) {
        UpdateCover(std::vector<unsigned char>(data.begin(), data.end()));
      }
    });
    if (cover_controller_) {
      Settings settings;
      settings.BeginGroup(CoversSettings::kSettingsGroup);
      const bool covers_automatic = settings.BoolValue(CoversSettings::kAutomaticSearch, CoversSettings::kDefaultAutomaticSearch);
      const bool context_enabled = context_view_ ? context_view_->search_cover_enabled() : ContextCover::LoadEnabled(settings);
      if (ContextCover::ShouldSearchForSong(context_enabled, covers_automatic, song.art_unset(), song.art_embedded(),
                                            song.art_automatic(), song.art_manual(), song.EffectiveAlbumartist(),
                                            ContextCover::EffectiveAlbum(song.album(), song.title()))) {
        if (playing_widget_) {
          playing_widget_->SearchCoverInProgress();
        }
        Song mutable_song = song;
        cover_controller_->SearchCoverAutomatically(&mutable_song, playing_widget_ ? playing_widget_->cover() : nullptr);
      }
    }
  }
}

void MainWindow::UpdateCover(const std::vector<unsigned char> &data) {
  if (playing_widget_) {
    playing_widget_->SetCover(data);
  }
  if (context_view_) {
    context_view_->AlbumCoverLoaded(data);
  }
  Appearance appearance;
  appearance.ReloadSettings();
  if (appearance.background_type() == static_cast<int>(AppearanceSettings::BackgroundImageType::Album)) {
    ApplyAppearance();
  }
}

void MainWindow::UpdatePlaybackButtons() {
  const bool playing = app_->player()->GetState() == GstEngine::State::Playing;
  gtk_button_set_icon_name(GTK_BUTTON(play_button_), playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic");
}

void MainWindow::OpenSettings(const char *page_name) {
  SettingsDialog::Show(GTK_WINDOW(window_), app_, [this]() {
    app_->scrobbler()->ReloadSettings();
    app_->shortcuts()->ReloadSettings();
    app_->player()->ReloadSettings();
    app_->network()->ReloadSettings();
    app_->osd()->ReloadSettings();
    app_->cover_providers()->ReloadSettings();
    app_->lyrics_providers()->ReloadSettings();
    ApplyAppearance();
    if (context_view_) {
      context_view_->ReloadSettings();
    }
    UpdateScrobblerButtons();
    ApplySeekbarMode();
    ApplyBehaviourSettings();
    ApplyPlaylistBehaviour();
    RefreshCollection();
    app_->analyzer()->ReloadSettings();
    ApplyAnalyzer();
    app_->moodbar()->ReloadSettings();
    app_->waveform()->ReloadSettings();
  }, page_name);
}

void MainWindow::OpenAbout() { AboutDialog::Show(GTK_WINDOW(window_)); }

void MainWindow::AddFiles() {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, Translations::CStr("Open audio files"));
  FileFilters::Apply(dialog, FileFilters::MediaFilters());
  gtk_file_dialog_open_multiple(dialog, GTK_WINDOW(window_), nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *self = static_cast<MainWindow *>(data);
    GError *error = nullptr;
    GListModel *files = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!files) {
      if (error) g_error_free(error);
      return;
    }
    std::vector<std::string> urls;
    const guint n = g_list_model_get_n_items(files);
    for (guint i = 0; i < n; ++i) {
      GFile *file = G_FILE(g_list_model_get_item(files, i));
      gchar *uri = g_file_get_uri(file);
      urls.emplace_back(uri);
      g_free(uri);
      g_object_unref(file);
    }
    self->app_->playlist_manager()->InsertUrls(urls);
    self->RefreshPlaylist();
    g_object_unref(files);
  }, this);
}

void MainWindow::AddCollectionFolder() {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, Translations::CStr("Add collection folder"));
  gtk_file_dialog_select_folder(dialog, GTK_WINDOW(window_), nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *self = static_cast<MainWindow *>(data);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!file) {
      if (error) g_error_free(error);
      return;
    }
    gchar *path = g_file_get_path(file);
    if (path) {
      self->app_->collection()->AddDirectory(path, true);
      self->RefreshCollection();
      g_free(path);
    }
    g_object_unref(file);
  }, this);
}

void MainWindow::LoadPlaylistFile() {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, Translations::CStr("Load playlist"));
  FileFilters::Apply(dialog, FileFilters::PlaylistFilters());
  gtk_file_dialog_open(dialog, GTK_WINDOW(window_), nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *self = static_cast<MainWindow *>(data);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!file) {
      if (error) g_error_free(error);
      return;
    }
    gchar *path = g_file_get_path(file);
    if (path) {
      self->app_->playlist_manager()->Load(path);
      self->RefreshPlaylistsList();
      self->RefreshPlaylist();
      g_free(path);
    }
    g_object_unref(file);
  }, this);
}

void MainWindow::SavePlaylistById(int id) {
  if (id < 0) {
    return;
  }
  app_->playlist_manager()->SetCurrentPlaylist(id);
  RefreshPlaylist();
  RefreshPlaylistTabs();
  SavePlaylistFile();
}

void MainWindow::SavePlaylistFile() {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, Translations::CStr("Save playlist"));
  FileFilters::Apply(dialog, FileFilters::PlaylistFilters());
  gtk_file_dialog_save(dialog, GTK_WINDOW(window_), nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *self = static_cast<MainWindow *>(data);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!file) {
      if (error) g_error_free(error);
      return;
    }
    gchar *path = g_file_get_path(file);
    if (!path || !self->app_->playlist_manager()->current()) {
      if (path) {
        g_free(path);
      }
      g_object_unref(file);
      return;
    }
    const std::string filename = path;
    g_free(path);
    g_object_unref(file);
    auto save = [self, filename]() {
      self->app_->playlist_manager()->Save(self->app_->playlist_manager()->current_id(), filename);
    };
    Settings settings;
    settings.BeginGroup(PlaylistSettings::kSettingsGroup);
    if (settings.IntValue(PlaylistSettings::kPathType, static_cast<int>(PlaylistSettings::kDefaultPathType)) ==
        static_cast<int>(PlaylistSettings::PathType::Ask_User)) {
      PlaylistSaveOptionsDialog::Show(GTK_WINDOW(self->window_), [save](PlaylistSaveOptionsDialog::PathType type) {
        ParserBase::SetPathTypeOverride(static_cast<int>(type));
        save();
        ParserBase::SetPathTypeOverride(-1);
      });
      return;
    }
    save();
  }, this);
}

void MainWindow::NewPlaylist() {
  Playlist *created = app_->playlist_manager()->New("Playlist " + std::to_string(app_->playlist_manager()->playlists().size() + 1));
  if (created && playlist_list_container_) {
    const std::string folder = playlist_list_container_->SelectedFolderPath();
    if (!folder.empty()) {
      app_->playlist_manager()->SetPlaylistUiPath(created->id(), folder);
    }
  }
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::ClearPlaylist() {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  if (!settings.BoolValue(PlaylistSettings::kPlaylistClear, PlaylistSettings::kDefaultPlaylistClear)) {
    return;
  }
  Playlist *playlist = app_->playlist_manager()->current();
  if (playlist && PlaylistUndoLimits::NeedsClearConfirmation(playlist->row_count())) {
    const std::string body = PlaylistUndoLimits::ClearConfirmBody(playlist->row_count());
    AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(PlaylistUndoLimits::ClearConfirmTitle()), body.c_str()));
    adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "clear", Translations::CStr("Clear"), nullptr);
    adw_alert_dialog_set_response_appearance(dialog, "clear", ADW_RESPONSE_DESTRUCTIVE);
    g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *response, gpointer data) {
                       if (g_strcmp0(response, "clear") != 0) {
                         return;
                       }
                       auto *self = static_cast<MainWindow *>(data);
                       self->app_->playlist_manager()->ClearCurrent();
                       self->RefreshPlaylist();
                     }),
                     this);
    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
    return;
  }
  app_->playlist_manager()->ClearCurrent();
  RefreshPlaylist();
}

void MainWindow::HideToTray() {
  if (app_->tray() && app_->tray()->available()) {
    SaveGeometry();
    gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
  }
}

void MainWindow::CloseCurrentPlaylist() {
  const int count = static_cast<int>(app_->playlist_manager()->playlists().size());
  if (PlaylistTabMenu::CloseCurrentHidesWindow(count)) {
    HideToTray();
    return;
  }
  TryClosePlaylist(app_->playlist_manager()->current_id());
}

void MainWindow::FinishClosePlaylist(int id) {
  if (id < 0) {
    return;
  }
  app_->playlist_manager()->Close(id);
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::TryClosePlaylist(int id) {
  Playlist *playlist = app_->playlist_manager()->playlist(id);
  if (!playlist) {
    return;
  }
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  const bool warn = settings.BoolValue(PlaylistSettings::kWarnClosePlaylist, PlaylistSettings::kDefaultWarnClosePlaylist);
  if (!PlaylistBehaviour::ShouldPromptClose(warn, playlist->favorite(), playlist->songs().empty())) {
    FinishClosePlaylist(id);
    return;
  }
  AdwAlertDialog *dialog =
      ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr("Remove playlist"),
                                           Translations::CStr("You are about to remove a playlist which is not part of your favorite "
                                                              "playlists: the playlist will be deleted (this action cannot be undone).\n"
                                                              "Are you sure you want to continue?")));
  GtkWidget *dont_warn = gtk_check_button_new_with_label(Translations::CStr("Warn me when closing a playlist tab"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(dont_warn), TRUE);
  adw_alert_dialog_set_extra_child(dialog, dont_warn);
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "close", Translations::CStr("Close"), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "close", ADW_RESPONSE_DESTRUCTIVE);
  g_object_set_data(G_OBJECT(dialog), "playlist-id", GINT_TO_POINTER(id + 1));
  g_object_set_data(G_OBJECT(dialog), "dont-warn", dont_warn);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "close") != 0) {
                       return;
                     }
                     auto *self = static_cast<MainWindow *>(data);
                     auto *check = GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(alert), "dont-warn"));
                     Settings persist;
                     persist.BeginGroup(PlaylistSettings::kSettingsGroup);
                     persist.SetBoolValue(PlaylistSettings::kWarnClosePlaylist, gtk_check_button_get_active(check));
                     persist.Sync();
                     const int playlist_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(alert), "playlist-id")) - 1;
                     self->FinishClosePlaylist(playlist_id);
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
}

void MainWindow::SelectPlayingTrack() {
  Playlist *active = app_->playlist_manager()->active();
  Playlist *current = app_->playlist_manager()->current();
  if (!active || active != current || active->current_row() < 0 || !playlist_container_) {
    return;
  }
  const int row = active->current_row();
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  if (settings.BoolValue(PlaylistSettings::kSelectTrack, PlaylistSettings::kDefaultSelectTrack)) {
    selected_playlist_rows_ = {row};
    selection_playlist_name_ = current->name();
    playlist_container_->view()->SetSelectedRows(selected_playlist_rows_);
    playlist_container_->view()->Refresh(current);
  }
  playlist_container_->view()->MaybeScrollToRow(row, Playlist::AutoScroll::Maybe);
}

void MainWindow::DeleteCurrentPlaylist() {
  if (app_->playlist_manager()->current_id() >= 0) {
    app_->playlist_manager()->Delete(app_->playlist_manager()->current_id());
    RefreshPlaylistsList();
    RefreshPlaylist();
  }
}

void MainWindow::RenameCurrentPlaylist() { RenamePlaylist(app_->playlist_manager()->current_id()); }

void MainWindow::RenamePlaylist(int id) {
  Playlist *playlist = app_->playlist_manager()->playlist(id);
  if (!playlist) {
    return;
  }
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Rename playlist", "Enter a new name for this playlist."));
  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), playlist->name().c_str());
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "rename", "Rename", nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "rename", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "rename");
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_object_set_data(G_OBJECT(dialog), "playlist-id", GINT_TO_POINTER(id + 1));
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "rename") != 0) {
                       return;
                     }
                     auto *self = static_cast<MainWindow *>(data);
                     auto *name_entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry"));
                     const std::string name = gtk_editable_get_text(name_entry);
                     const int playlist_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(alert), "playlist-id")) - 1;
                     if (!name.empty() && playlist_id >= 0) {
                       self->app_->playlist_manager()->Rename(playlist_id, name);
                       self->RefreshPlaylistsList();
                       self->RefreshPlaylist();
                     }
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
}

void MainWindow::UndoPlaylist() {
  if (Playlist *playlist = app_->playlist_manager()->current()) {
    playlist->Undo();
    app_->playlist_manager()->SaveCurrent();
    RefreshPlaylist();
  }
}

void MainWindow::RedoPlaylist() {
  if (Playlist *playlist = app_->playlist_manager()->current()) {
    playlist->Redo();
    app_->playlist_manager()->SaveCurrent();
    RefreshPlaylist();
  }
}

void MainWindow::ApplyAnalyzer() {
  if (!analyzer_drawing_) {
    return;
  }
  gtk_widget_set_visible(analyzer_drawing_, app_->analyzer()->enabled());
  gtk_widget_set_tooltip_text(analyzer_drawing_, ("Analyzer: " + app_->analyzer()->type()).c_str());
  EnsureAnalyzerTimer();
  gtk_widget_queue_draw(analyzer_drawing_);
}

void MainWindow::EnsureAnalyzerTimer() {
  if (!app_->analyzer()->enabled()) {
    if (analyzer_timeout_) {
      g_source_remove(analyzer_timeout_);
      analyzer_timeout_ = 0;
    }
    analyzer_timer_ms_ = 0;
    return;
  }
  const int ms = std::max(1, 1000 / std::max(1, app_->analyzer()->framerate()));
  if (analyzer_timeout_ != 0 && analyzer_timer_ms_ == ms) {
    return;
  }
  if (analyzer_timeout_) {
    g_source_remove(analyzer_timeout_);
    analyzer_timeout_ = 0;
  }
  analyzer_timer_ms_ = ms;
  analyzer_timeout_ = g_timeout_add_full(G_PRIORITY_DEFAULT, static_cast<guint>(ms),
                                         +[](gpointer data) -> gboolean {
                                           static_cast<MainWindow *>(data)->TickAnalyzer();
                                           return G_SOURCE_CONTINUE;
                                         },
                                         this, nullptr);
}

void MainWindow::TickAnalyzer() {
  if (!analyzer_drawing_ || !app_->analyzer()->enabled()) {
    return;
  }
  const auto state = app_->player()->GetState();
  app_->analyzer()->set_paused(state == GstEngine::State::Paused);
  app_->analyzer()->set_playing(state == GstEngine::State::Playing);
  gtk_widget_queue_draw(analyzer_drawing_);
}

void MainWindow::CycleAnalyzer() {
  app_->analyzer()->set_type(Analyzer::NextType(app_->analyzer()->type()));
  ApplyAnalyzer();
}

void MainWindow::ShowAnalyzerMenu() {
  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, analyzer_drawing_);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);
  GtkWidget *enable = gtk_check_button_new_with_label(Translations::CStr("Show analyzer"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(enable), app_->analyzer()->enabled());
  g_signal_connect(enable, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     self->app_->analyzer()->set_enabled(gtk_check_button_get_active(button));
                     self->ApplyAnalyzer();
                   }),
                   this);
  gtk_box_append(GTK_BOX(box), enable);
  for (const std::string &type : Analyzer::Types()) {
    GtkWidget *button = gtk_button_new_with_label(type.c_str());
    g_object_set_data_full(G_OBJECT(button), "analyzer-type", g_strdup(type.c_str()), g_free);
    g_object_set_data(G_OBJECT(button), "popover", popover);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       const char *type_name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "analyzer-type"));
                       if (type_name) {
                         self->app_->analyzer()->set_type(type_name);
                         self->ApplyAnalyzer();
                       }
                       if (auto *pop = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "popover"))) {
                         gtk_popover_popdown(GTK_POPOVER(pop));
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(box), button);
  }
  GtkWidget *fps_label = gtk_label_new(Translations::CStr("Framerate"));
  gtk_widget_set_halign(fps_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), fps_label);
  GtkWidget *fps_group = nullptr;
  for (const AnalyzerFramerate::Preset &preset : AnalyzerFramerate::Presets()) {
    GtkWidget *choice = gtk_check_button_new_with_label(preset.label);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(choice), app_->analyzer()->framerate() == preset.fps);
    if (fps_group) {
      gtk_check_button_set_group(GTK_CHECK_BUTTON(choice), GTK_CHECK_BUTTON(fps_group));
    } else {
      fps_group = choice;
    }
    g_object_set_data(G_OBJECT(choice), "fps", GINT_TO_POINTER(preset.fps));
    g_object_set_data(G_OBJECT(choice), "popover", popover);
    g_signal_connect(choice, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                       if (!gtk_check_button_get_active(button)) {
                         return;
                       }
                       auto *self = static_cast<MainWindow *>(data);
                       self->app_->analyzer()->set_framerate(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "fps")));
                       self->EnsureAnalyzerTimer();
                       if (auto *pop = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "popover"))) {
                         gtk_popover_popdown(GTK_POPOVER(pop));
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(box), choice);
  }
  gtk_popover_set_child(GTK_POPOVER(popover), box);
  gtk_popover_popup(GTK_POPOVER(popover));
}

void MainWindow::CycleRepeat() {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return;
  }
  playlist_sequence_.CycleRepeatMode();
  playlist->SetRepeatMode(playlist_sequence_.repeat_mode());
  if (playlist_container_) {
    playlist_container_->SetRepeatMode(playlist_sequence_.repeat_mode());
  }
}

void MainWindow::CycleShuffle() {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return;
  }
  playlist_sequence_.CycleShuffleMode();
  playlist->SetShuffleMode(playlist_sequence_.shuffle_mode());
  if (playlist_container_) {
    playlist_container_->SetShuffleMode(playlist_sequence_.shuffle_mode());
  }
  if (playlist_sequence_.shuffle_mode() == PlaylistSequence::ShuffleMode::All) {
    app_->playlist_manager()->ShuffleCurrent();
    RefreshPlaylist();
  }
}

void MainWindow::RunSmartPlaylist(const std::string &kind) {
  app_->playlist_manager()->PlaySmartPlaylist(kind, true, true);
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::PlayRadioChannel(const RadioChannel &channel) {
  const Song song = RadioStreamPlaylistItem(channel).EffectiveMetadata();
  app_->playlist_manager()->AppendSongs({song});
  RefreshPlaylist();
  if (app_->playlist_manager()->active()) {
    app_->player()->PlayAt(app_->playlist_manager()->active()->row_count() - 1);
  }
}

void MainWindow::ApplyBehaviourSettings() {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  if (playing_widget_) {
    playing_widget_->SetEnabled(settings.BoolValue(BehaviourSettings::kPlayingWidget, BehaviourSettings::kDefaultPlayingWidget));
  }
  ApplyPlaylistBehaviour();
}

void MainWindow::ApplyPlaylistBehaviour() {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  const bool auto_sort = settings.BoolValue(PlaylistSettings::kAutoSort, PlaylistSettings::kDefaultAutoSort);
  for (Playlist *playlist : app_->playlist_manager()->GetAllPlaylists()) {
    playlist->set_auto_sort(auto_sort);
    if (sort_column_ != PlaylistColumn::Count) {
      playlist->SetSort(sort_column_, sort_descending_);
    }
  }
  if (app_->player()) {
    app_->player()->ReloadSettings();
  }
  if (playlist_container_) {
    playlist_container_->ApplyLook();
  }
}

void MainWindow::ApplyAppearance() {
  Appearance appearance;
  appearance.Apply();
  std::string cover_path;
  if (appearance.background_type() == static_cast<int>(AppearanceSettings::BackgroundImageType::Album) && app_) {
    const Song song = app_->player()->current_song();
    cover_path = FileUtils::PathFromUri(song.art_manual().empty() ? song.art_automatic() : song.art_manual());
  }
  const std::string css = appearance.BackgroundCss(cover_path);
  if (!css.empty()) {
    StyleUtils::LoadCss(css);
  }
  if (play_button_) {
    const int size = appearance.icon_sizes().play_controls;
    for (GtkWidget *child = gtk_widget_get_first_child(gtk_widget_get_parent(play_button_)); child; child = gtk_widget_get_next_sibling(child)) {
      if (!GTK_IS_BUTTON(child)) {
        continue;
      }
      if (GtkWidget *image = gtk_button_get_child(GTK_BUTTON(child)); image && GTK_IS_IMAGE(image)) {
        gtk_image_set_pixel_size(GTK_IMAGE(image), size);
      }
    }
  }
}

void MainWindow::RestoreGeometry() {
  Settings settings;
  settings.BeginGroup(WindowGeometry::kSettingsGroup);
  const WindowGeometry::State state = WindowGeometry::FromValues(settings.IntValue(WindowGeometry::kWidth, WindowGeometry::kDefaultWidth),
                                                                 settings.IntValue(WindowGeometry::kHeight, WindowGeometry::kDefaultHeight),
                                                                 settings.BoolValue(WindowGeometry::kMaximized, false));
  gtk_window_set_default_size(GTK_WINDOW(window_), state.width, state.height);
  Settings behaviour;
  behaviour.BeginGroup(BehaviourSettings::kSettingsGroup);
  const int action = WindowGeometry::StartupAction(
      behaviour.IntValue(BehaviourSettings::kStartupBehaviour, static_cast<int>(BehaviourSettings::kDefaultStartupBehaviour)), state.maximized);
  if (action == 4) {
    gtk_window_maximize(GTK_WINDOW(window_));
  } else if (action == 5) {
    gtk_window_minimize(GTK_WINDOW(window_));
  } else if (action == 3) {
    gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
  }
}

void MainWindow::SetShowSidebar(bool show) {
  Settings settings;
  settings.BeginGroup(MainWindowSettings::kSettingsGroup);
  settings.SetBoolValue(MainWindowSettings::kShowSidebar, show);
  settings.Sync();
  if (sidebar_action_) {
    g_simple_action_set_state(sidebar_action_, g_variant_new_boolean(show));
  }
  ApplySidebar();
}

void MainWindow::ApplySidebar() {
  if (!split_view_) {
    return;
  }
  Settings settings;
  settings.BeginGroup(MainWindowSettings::kSettingsGroup);
  const bool show = MainWindowLook::ShowSidebar(settings.BoolValue(MainWindowSettings::kShowSidebar, MainWindowSettings::kDefaultShowSidebar));
  adw_overlay_split_view_set_show_sidebar(ADW_OVERLAY_SPLIT_VIEW(split_view_), show);
}

void MainWindow::ToggleMute() { app_->player()->Mute(); }

void MainWindow::ApplyMuteUi(unsigned volume) {
  Settings settings;
  settings.BeginGroup(BackendSettings::kSettingsGroup);
  const bool visible = MainWindowLook::MuteVisible(settings.BoolValue(BackendSettings::kVolumeControl, BackendSettings::kDefaultVolumeControl));
  const bool muted = MainWindowLook::IsMuted(volume);
  if (mute_button_) {
    gtk_widget_set_visible(mute_button_, visible);
    gtk_button_set_icon_name(GTK_BUTTON(mute_button_), MainWindowLook::MuteIconName(muted));
    gtk_widget_set_tooltip_text(mute_button_, Translations::CStr(MainWindowLook::MuteTooltip(muted)));
    if (muted) {
      gtk_widget_add_css_class(mute_button_, "accent");
    } else {
      gtk_widget_remove_css_class(mute_button_, "accent");
    }
  }
  if (mute_action_) {
    g_simple_action_set_enabled(mute_action_, visible);
    g_simple_action_set_state(mute_action_, g_variant_new_boolean(muted));
  }
}

void MainWindow::SaveGeometry() {
  if (!window_) {
    return;
  }
  int width = gtk_widget_get_width(GTK_WIDGET(window_));
  int height = gtk_widget_get_height(GTK_WIDGET(window_));
  if (width <= 0 || height <= 0) {
    gtk_window_get_default_size(GTK_WINDOW(window_), &width, &height);
  }
  const WindowGeometry::State state = WindowGeometry::FromValues(width, height, gtk_window_is_maximized(GTK_WINDOW(window_)) == TRUE);
  Settings settings;
  settings.BeginGroup(WindowGeometry::kSettingsGroup);
  settings.SetIntValue(WindowGeometry::kWidth, state.width);
  settings.SetIntValue(WindowGeometry::kHeight, state.height);
  settings.SetBoolValue(WindowGeometry::kMaximized, state.maximized);
  settings.Sync();
}

bool MainWindow::EngineStopped() const {
  const GstEngine::State state = app_->player()->GetState();
  return state != GstEngine::State::Playing && state != GstEngine::State::Paused;
}

SongList MainWindow::CollectionSongs() const {
  return collection_container_ ? collection_container_->view()->SelectedSongs() : SongList{};
}

void MainWindow::ApplyCollectionPlan(const CollectionBehaviour::Plan &plan, const SongList &songs) {
  if (songs.empty()) {
    return;
  }
  int play_at = 0;
  if (plan.destination == CollectionBehaviour::Destination::New) {
    app_->playlist_manager()->New(CollectionBehaviour::NewPlaylistName(songs), songs);
  } else {
    if (plan.clear_current) {
      app_->playlist_manager()->ClearCurrent();
    }
    if (Playlist *playlist = app_->playlist_manager()->current()) {
      play_at = plan.clear_current ? 0 : playlist->row_count();
    }
    app_->playlist_manager()->AppendSongs(songs);
  }
  if (plan.queue == CollectionBehaviour::QueueMode::Append) {
    for (const Song &song : songs) {
      app_->queue()->Append(song);
    }
  } else if (plan.queue == CollectionBehaviour::QueueMode::Next) {
    for (auto it = songs.rbegin(); it != songs.rend(); ++it) {
      app_->queue()->InsertNext(*it);
    }
  }
  RefreshPlaylistsList();
  RefreshPlaylist();
  RefreshQueue();
  if (plan.should_play) {
    if (plan.destination == CollectionBehaviour::Destination::New && app_->playlist_manager()->current()) {
      app_->player()->PlayPlaylist(app_->playlist_manager()->current()->name());
    } else {
      app_->player()->PlayAt(play_at);
    }
  }
}

void MainWindow::ForceCompilationSelected(bool on) {
  const SongList songs = CollectionSongs();
  if (songs.empty() || !app_->collection() || !app_->collection()->backend()) {
    return;
  }
  app_->collection()->backend()->ForceCompilation(songs, on);
  RefreshCollection();
  ShowToast(on ? "Shown in Various artists" : "Removed from Various artists");
}

void MainWindow::ShowCollectionMenu() {
  bool devices_connected = false;
  for (const ConnectedDevice &device : app_->device_manager()->devices()) {
    if (!device.mount_path.empty()) {
      devices_connected = true;
      break;
    }
  }
  const CollectionMenu::SelectionState state =
      CollectionMenu::Analyze(CollectionSongs(), DeleteFilesPolicy::Allowed(DeleteFilesPolicy::Source::Collection), devices_connected);
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::Tr("Expand all").c_str(), "win.collection-expand-all");
  g_menu_append(menu, Translations::Tr("Collapse all").c_str(), "win.collection-collapse-all");
  g_menu_append(menu, Translations::Tr(SettingsPages::ConfigureCollectionLabel()).c_str(), "win.collection-configure");
  for (const CollectionMenu::Item &item : CollectionMenu::VisibleItems(state)) {
    if (item.action == CollectionMenu::Action::GroupBy) {
      GMenu *group_by = g_menu_new();
      const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
      for (size_t i = 0; i < presets.size(); ++i) {
        if (presets[i].advanced) {
          g_menu_append(group_by, Translations::Tr("Advanced grouping…").c_str(), "win.group-by");
          continue;
        }
        const std::string target = "win.collection-preset(" + std::to_string(static_cast<int>(i)) + ")";
        g_menu_append(group_by, Translations::Tr(presets[i].label).c_str(), target.c_str());
      }
      g_menu_append_submenu(menu, Translations::Tr(CollectionMenu::LabelFor(item.action)).c_str(), G_MENU_MODEL(group_by));
      continue;
    }
    g_menu_append(menu, Translations::Tr(CollectionMenu::LabelFor(item.action)).c_str(), CollectionMenu::WinAction(item.action));
  }
  CollectionFilterWidget *collection_filter = collection_container_ ? collection_container_->filter_widget() : nullptr;
  if (CollectionMenu::DisplayOptionsEnabled() && collection_filter && collection_filter->MenuModel()) {
    g_menu_append_submenu(menu, Translations::Tr(CollectionMenu::DisplayOptionsLabel()).c_str(), collection_filter->MenuModel());
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, collection_container_ ? collection_container_->view()->list() : GTK_WIDGET(window_));
  if (collection_filter) {
    collection_filter->AttachActions(popover);
  }
  gtk_popover_popup(GTK_POPOVER(popover));
}

void MainWindow::ShowStreamingMenu(const SongList &songs, StreamingCollectionActions::MenuContext ctx) {
  streaming_menu_songs_ = songs;
  const int songs_selected = static_cast<int>(streaming_menu_songs_.size());
  StreamingTabsView *tabs = CurrentStreamingTabs();
  CollectionFilterWidget *filter = tabs ? tabs->CurrentFilterWidget() : nullptr;
  StreamingSearchView *search = tabs ? tabs->search_view() : nullptr;
  GMenu *menu = g_menu_new();
  for (const StreamingCollectionActions::Item &item : StreamingCollectionActions::VisibleItems(songs_selected, ctx)) {
    g_menu_append(menu, Translations::Tr(item.label).c_str(), item.action);
  }
  if (StreamingCollectionActions::SearchForThisEnabled(songs_selected, ctx)) {
    g_menu_append(menu, Translations::Tr(StreamingSearchOpts::SearchForThisLabel()).c_str(), "win.streaming-search");
  }
  if (StreamingCollectionActions::SearchContextActionsEnabled(songs_selected, ctx)) {
    for (StreamingCollectionStore::List list : StreamingCollectionStore::AddableLists(streaming_service_name_)) {
      const char *action = "win.streaming-add-songs";
      if (list == StreamingCollectionStore::List::Artists) {
        action = "win.streaming-add-artists";
      } else if (list == StreamingCollectionStore::List::Albums) {
        action = "win.streaming-add-albums";
      }
      g_menu_append(menu, Translations::Tr(StreamingCollectionStore::AddLabel(list)).c_str(), action);
    }
  }
  if (StreamingCollectionActions::DisplayOptionsEnabled(ctx) && filter && filter->MenuModel()) {
    g_menu_append_submenu(menu, Translations::Tr(StreamingCollectionActions::DisplayOptionsLabel()).c_str(), filter->MenuModel());
  }
  if (StreamingCollectionActions::SearchSettingsEnabled(ctx) && search) {
    if (search->GroupMenuModel()) {
      g_menu_append_submenu(menu, Translations::Tr(StreamingCollectionActions::SearchGroupByLabel()).c_str(), search->GroupMenuModel());
    }
    const std::string configure = StreamingSearchOpts::ConfigureServiceLabel(streaming_service_name_);
    g_menu_append(menu, Translations::Tr(configure.c_str()).c_str(), "win.streaming-configure");
  }
  if (g_menu_model_get_n_items(G_MENU_MODEL(menu)) == 0) {
    g_object_unref(menu);
    return;
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  GtkWidget *parent = streaming_stack_ ? streaming_stack_ : GTK_WIDGET(window_);
  gtk_widget_set_parent(popover, parent);
  if (filter) {
    filter->AttachActions(popover);
  }
  if (search) {
    search->AttachGroupActions(popover);
  }
  gtk_popover_popup(GTK_POPOVER(popover));
}

StreamingTabsView *MainWindow::CurrentStreamingTabs() const {
  for (const auto &view : streaming_views_) {
    if (view && view->service() && view->service()->name() == streaming_service_name_) {
      return view.get();
    }
  }
  return nullptr;
}

void MainWindow::StreamingFavorite(bool add) {
  StreamingService *service = app_->streaming_services()->ServiceByName(streaming_service_name_);
  if (!service || streaming_menu_songs_.empty()) {
    return;
  }
  StreamingTabsView *tabs = nullptr;
  for (const auto &view : streaming_views_) {
    if (view && view->service() && view->service()->name() == streaming_service_name_) {
      tabs = view.get();
      break;
    }
  }
  StreamingCollectionStore::List store_list = StreamingCollectionStore::List::Songs;
  const bool remove_store = !add && tabs && tabs->CurrentStoreList(&store_list);
  if (remove_store) {
    tabs->RemoveFromCollection(store_list, streaming_menu_songs_);
  }
  const auto type = remove_store ? StreamingFavoriteAction::TypeFromList(store_list) : StreamingFavoriteAction::TypeForSongs(streaming_menu_songs_);
  auto done = [this, add](const SongList &) {
    ShowToast(add ? "Added to favorites" : "Removed from favorites");
    for (const auto &view : streaming_views_) {
      if (view && view->service() && view->service()->name() == streaming_service_name_) {
        view->GetFavorites();
      }
    }
  };
  if (add) {
    service->AddFavorites(type, streaming_menu_songs_, done);
  } else {
    service->RemoveFavorites(type, streaming_menu_songs_, done);
  }
}

void MainWindow::StreamingSearchForThis() {
  StreamingTabsView *tabs = nullptr;
  for (const auto &view : streaming_views_) {
    if (view && view->service() && view->service()->name() == streaming_service_name_) {
      tabs = view.get();
      break;
    }
  }
  if (!tabs) {
    return;
  }
  std::string query = tabs->SelectedSearchQuery();
  if (!StreamingSearchOpts::CanSearchForThis(query) && !streaming_menu_songs_.empty()) {
    query = StreamingSearchOpts::QueryFromSong(streaming_menu_songs_.front(), StreamingService::SearchType::Songs);
  }
  tabs->SearchForThis(query);
}

void MainWindow::StreamingAddToList(StreamingCollectionStore::List list) {
  if (streaming_menu_songs_.empty() || !StreamingCollectionStore::CanStore(streaming_service_name_, list)) {
    return;
  }
  StreamingService *service = app_->streaming_services()->ServiceByName(streaming_service_name_);
  for (const auto &view : streaming_views_) {
    if (view && view->service() && view->service()->name() == streaming_service_name_) {
      view->AddToCollection(list, streaming_menu_songs_);
      break;
    }
  }
  if (service) {
    service->AddFavorites(StreamingFavoriteAction::TypeFromList(list), streaming_menu_songs_, {});
  }
  ShowToast(StreamingCollectionStore::AddedStatus(list));
}

void MainWindow::ShowRadioMenu(const std::vector<RadioChannel> &channels) {
  radio_menu_channels_ = channels;
  radio_menu_songs_ = RadioDrag::Songs(channels);
  GMenu *menu = g_menu_new();
  for (const RadioMenu::Item &item : RadioMenu::VisibleItems(!channels.empty())) {
    g_menu_append(menu, Translations::Tr(item.label).c_str(), RadioMenu::WinAction(item.action));
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  GtkWidget *parent = radio_container_ && radio_container_->view() ? radio_container_->view()->list() : GTK_WIDGET(window_);
  gtk_widget_set_parent(popover, parent);
  gtk_popover_popup(GTK_POPOVER(popover));
}

void MainWindow::ShowPlaylistListMenu(const std::string &name) {
  playlist_list_menu_name_ = name;
  playlist_list_menu_folder_.clear();
  if (playlist_list_container_ && playlist_list_container_->SelectedIsFolder()) {
    playlist_list_menu_folder_ = playlist_list_container_->SelectedFolderPath();
  }
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::Tr("New folder").c_str(), "win.playlist-list-new-folder");
  if (!playlist_list_menu_folder_.empty()) {
    g_menu_append(menu, Translations::Tr("Rename folder…").c_str(), "win.playlist-list-rename-folder");
    g_menu_append(menu, Translations::Tr("Delete folder").c_str(), "win.playlist-list-delete-folder");
  } else if (!playlist_list_menu_name_.empty()) {
    Playlist *playlist = PlaylistByName(playlist_list_menu_name_);
    g_menu_append(menu, Translations::Tr("Open").c_str(), "win.playlist-list-open");
    g_menu_append(menu, playlist && playlist->favorite() ? Translations::Tr("Remove from favorites").c_str() : Translations::Tr("Add to favorites").c_str(),
                  "win.playlist-list-favorite");
    g_menu_append(menu, Translations::Tr("Rename…").c_str(), "win.playlist-list-rename");
    g_menu_append(menu, Translations::Tr("Delete").c_str(), "win.playlist-list-delete");
    g_menu_append(menu, Translations::Tr("Save playlist").c_str(), "win.playlist-list-save");
    g_menu_append(menu, Translations::Tr("Copy to device…").c_str(), "win.playlist-list-copy");
    if (playlist && !playlist->ui_path().empty()) {
      g_menu_append(menu, Translations::Tr("Remove from folder").c_str(), "win.playlist-list-remove-from-folder");
    }
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  GtkWidget *parent = playlist_list_container_ && playlist_list_container_->view() ? playlist_list_container_->view()->list() : GTK_WIDGET(window_);
  gtk_widget_set_parent(popover, parent);
  gtk_popover_popup(GTK_POPOVER(popover));
}

void MainWindow::SelectPlaylistByName(const std::string &name) {
  if (name.empty()) {
    return;
  }
  app_->playlist_manager()->SetCurrentPlaylist(name);
  RefreshPlaylist();
  RefreshPlaylistTabs();
  RefreshPlaylistsList();
}

Playlist *MainWindow::PlaylistByName(const std::string &name) const {
  if (name.empty()) {
    return nullptr;
  }
  for (Playlist *playlist : app_->playlist_manager()->GetAllPlaylists()) {
    if (playlist && playlist->name() == name) {
      return playlist;
    }
  }
  return nullptr;
}

void MainWindow::DropOnPlaylistList(const std::string &name, const std::string &payload, bool folder) {
  if (PlaylistListDrop::IsPlaylistMove(payload)) {
    const std::string source = PlaylistListDrop::ParseMoveName(payload);
    if (source.empty() || source == name) {
      return;
    }
    std::string dest;
    if (folder) {
      dest = name;
    } else if (Playlist *target = PlaylistByName(name)) {
      dest = target->ui_path();
    }
    MovePlaylistToFolder(source, dest);
    return;
  }
  if (folder) {
    return;
  }
  Playlist *target = PlaylistByName(name);
  if (!target) {
    return;
  }
  if (PlaylistListDrop::IsPlaylistRows(payload)) {
    const PlaylistDragPayload::Payload drag = PlaylistDragPayload::Decode(payload);
    const int source_id = drag.source_id >= 0 ? drag.source_id : app_->playlist_manager()->current_id();
    if (!PlaylistListLook::ShouldAcceptPlaylistRowsDrop(target->id(), source_id)) {
      return;
    }
    app_->playlist_manager()->MoveRowsBetween(source_id, target->id(), drag.rows);
  } else {
    const std::vector<std::string> urls = PlaylistListDrop::ParseUrls(payload);
    if (urls.empty()) {
      return;
    }
    app_->playlist_manager()->SetCurrentPlaylist(name);
    app_->playlist_manager()->InsertUrls(urls);
  }
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::NewPlaylistFolder() {
  if (!playlist_list_container_) {
    return;
  }
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr("New folder"), Translations::CStr("Enter the name of the folder")));
  GtkWidget *entry = gtk_entry_new();
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "create", Translations::CStr("Create"), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "create", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "create");
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "create") != 0) {
                       return;
                     }
                     auto *self = static_cast<MainWindow *>(data);
                     auto *name_entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry"));
                     const std::string name = PlaylistFolders::SanitizeName(gtk_editable_get_text(name_entry));
                     if (name.empty() || !self->playlist_list_container_) {
                       return;
                     }
                     const std::string parent = self->playlist_list_container_->SelectedFolderPath();
                     self->playlist_list_container_->AddExtraFolder(PlaylistFolders::Child(parent, name));
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
}

void MainWindow::RenamePlaylistFolder(const std::string &path) {
  if (path.empty() || !playlist_list_container_) {
    return;
  }
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr("Rename folder"), Translations::CStr("Enter a new name for this folder.")));
  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), PlaylistFolders::Leaf(path).c_str());
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "rename", Translations::CStr("Rename"), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "rename", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "rename");
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_object_set_data_full(G_OBJECT(dialog), "folder-path", g_strdup(path.c_str()), g_free);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "rename") != 0) {
                       return;
                     }
                     auto *self = static_cast<MainWindow *>(data);
                     auto *name_entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry"));
                     const char *old_path = static_cast<const char *>(g_object_get_data(G_OBJECT(alert), "folder-path"));
                     const std::string name = PlaylistFolders::SanitizeName(gtk_editable_get_text(name_entry));
                     if (name.empty() || !old_path || !self->playlist_list_container_) {
                       return;
                     }
                     const std::string new_path = PlaylistFolders::Child(PlaylistFolders::Parent(old_path), name);
                     if (new_path == old_path) {
                       return;
                     }
                     for (Playlist *playlist : self->app_->playlist_manager()->GetAllPlaylists()) {
                       if (!playlist) {
                         continue;
                       }
                       const std::string updated = PlaylistFolders::RenamePrefix(playlist->ui_path(), old_path, new_path);
                       if (updated != playlist->ui_path()) {
                         self->app_->playlist_manager()->SetPlaylistUiPath(playlist->id(), updated);
                       }
                     }
                     self->playlist_list_container_->RenameExtraFolder(old_path, new_path);
                     self->RefreshPlaylistsList();
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
}

void MainWindow::DeletePlaylistFolder(const std::string &path) {
  if (path.empty() || !playlist_list_container_) {
    return;
  }
  const std::string parent = PlaylistFolders::Parent(path);
  for (Playlist *playlist : app_->playlist_manager()->GetAllPlaylists()) {
    if (playlist && PlaylistFolders::IsUnder(playlist->ui_path(), path)) {
      app_->playlist_manager()->SetPlaylistUiPath(playlist->id(), parent);
    }
  }
  playlist_list_container_->RemoveExtraFolder(path);
  RefreshPlaylistsList();
}

void MainWindow::MovePlaylistToFolder(const std::string &name, const std::string &folder) {
  Playlist *playlist = PlaylistByName(name);
  if (!playlist) {
    return;
  }
  app_->playlist_manager()->SetPlaylistUiPath(playlist->id(), folder);
  RefreshPlaylistsList();
}

SongList MainWindow::SongsFromUrls(const std::vector<std::string> &urls) const {
  SongList songs;
  Playlist *playlist = app_->playlist_manager()->current();
  for (const std::string &url : urls) {
    Song song;
    if (playlist) {
      for (int i = 0; i < playlist->row_count(); ++i) {
        if (playlist->song(i).url() == url) {
          song = playlist->song(i);
          break;
        }
      }
    }
    if (song.url().empty()) {
      song.set_url(url);
      song.set_title(url);
      song.set_valid(true);
    }
    songs.push_back(song);
  }
  return songs;
}

void MainWindow::ShowPlaylistMenu(double, double y) {
  Playlist *playlist = app_->playlist_manager()->current();
  const std::vector<int> rows = SelectedPlaylistRows();
  playlist_menu_row_ = -1;
  if (playlist && playlist_container_ && playlist_container_->view()) {
    playlist_menu_row_ =
        PlaylistMenu::ContextRow(playlist_container_->view()->RowAtY(y, playlist_container_->view()->grid()), playlist->row_count());
  }
  const PlaylistColumn column = playlist_container_ ? playlist_container_->view()->last_clicked_column() : PlaylistColumn::Title;
  PlaylistMenu::SelectionState opts;
  opts.index_valid = playlist_menu_row_ >= 0;
  opts.track_column = column == PlaylistColumn::Track;
  opts.column_editable = PlaylistDelegates::ColumnIsEditable(column);
  opts.column = column;
  opts.column_name = PlaylistDelegates::ColumnTitle(column);
  opts.delete_allowed = DeleteFilesPolicy::Allowed(DeleteFilesPolicy::Source::Playlist);
  for (const ConnectedDevice &device : app_->device_manager()->devices()) {
    if (!device.mount_path.empty()) {
      opts.devices_connected = true;
      break;
    }
  }
  const int info_row = playlist_menu_row_ >= 0 ? playlist_menu_row_ : (rows.empty() ? -1 : rows.front());
  if (playlist && info_row >= 0 && info_row < playlist->row_count()) {
    const Song &song = playlist->song(info_row);
    opts.column_value = PlaylistDelegates::ColumnText(song, column);
    opts.collection_item = song.source() == Song::Source::Collection && song.id() > 0;
    opts.playing_selected = playlist->current_row() == info_row && app_->player()->GetState() == GstEngine::State::Playing;
    PlaylistMenu::ApplyContextSong(&opts, song, opts.playing_selected);
  }
  std::vector<PlaylistMenu::RowInfo> infos;
  if (playlist) {
    for (int row : rows) {
      if (row < 0 || row >= playlist->row_count()) {
        continue;
      }
      infos.push_back(PlaylistMenu::FromSong(playlist->song(row), app_->queue()->ContainsPlaylistRow(playlist->id(), row)));
    }
  }
  const PlaylistMenu::SelectionState state = PlaylistMenu::Analyze(infos, opts);

  GMenu *menu = g_menu_new();
  for (const PlaylistMenu::Item &item : PlaylistMenu::VisibleItems(state)) {
    if (item.action == PlaylistMenu::Action::AddToPlaylist) {
      GMenu *add_to = g_menu_new();
      for (Playlist *other : app_->playlist_manager()->GetAllPlaylists()) {
        if (!other || other == playlist) {
          continue;
        }
        const std::string target = "win.add-to-playlist(" + std::to_string(other->id()) + ")";
        g_menu_append(add_to, other->name().c_str(), target.c_str());
      }
      g_menu_append(add_to, Translations::Tr("New playlist").c_str(), "win.add-to-playlist(-1)");
      g_menu_append_submenu(menu, Translations::Tr(PlaylistMenu::LabelFor(item.action, state)).c_str(), G_MENU_MODEL(add_to));
      continue;
    }
    g_menu_append(menu, Translations::Tr(PlaylistMenu::LabelFor(item.action, state)).c_str(), PlaylistMenu::WinAction(item.action));
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, playlist_container_ ? playlist_container_->view()->grid() : GTK_WIDGET(window_));
  gtk_popover_popup(GTK_POPOVER(popover));
}

std::vector<int> MainWindow::SelectedPlaylistRows() const {
  if (!selected_playlist_rows_.empty()) {
    return selected_playlist_rows_;
  }
  if (Playlist *playlist = app_->playlist_manager()->current()) {
    if (playlist->current_row() >= 0) {
      return {playlist->current_row()};
    }
  }
  return {};
}

SongList MainWindow::SelectedSongs() const {
  SongList songs;
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return songs;
  }
  for (int row : SelectedPlaylistRows()) {
    if (row >= 0 && row < playlist->row_count()) {
      songs.push_back(playlist->songs()[static_cast<size_t>(row)]);
    }
  }
  return songs;
}

void MainWindow::ShowToast(const std::string &text) {
  if (toast_overlay_) {
    adw_toast_overlay_add_toast(toast_overlay_, adw_toast_new(text.c_str()));
  }
}

void MainWindow::AddCdTracks() {
  SongList songs = CddaSongLoader().LoadDevice({});
  if (songs.empty()) {
    for (const ConnectedDevice &device : app_->device_manager()->devices()) {
      if (device.backend == "cdda") {
        const SongList tracks = app_->device_manager()->Songs(device.unique_id);
        songs.insert(songs.end(), tracks.begin(), tracks.end());
      }
    }
  }
  if (songs.empty()) {
    ShowToast("No audio CD found");
    return;
  }
  app_->playlist_manager()->New("Audio CD", songs);
  RefreshPlaylistsList();
  RefreshPlaylist();
  ShowToast("Added " + std::to_string(songs.size()) + " CD tracks");
}

void MainWindow::RescanCollection(bool full) {
  if (app_->collection()->scanning()) {
    ShowToast("A collection scan is already running");
    return;
  }
  if (full) {
    app_->collection()->FullScan();
  } else {
    app_->collection()->IncrementalScan();
  }
  ShowToast(full ? "Full collection scan started" : "Collection rescan started");
}

void MainWindow::StopAfterCurrent() {
  app_->player()->StopAfterCurrent();
  app_->osd()->StopAfterToggle(app_->player()->stop_after_current());
  RefreshPlaylist();
  ShowToast(app_->player()->stop_after_current() ? "Will stop after the current track" : "Stop after this track cancelled");
}

void MainWindow::PlayPlaylistMenuRow() {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist || playlist_menu_row_ < 0 || playlist_menu_row_ >= playlist->row_count()) {
    return;
  }
  PlaylistMenu::SelectionState state;
  state.index_valid = true;
  state.playing_selected = playlist->current_row() == playlist_menu_row_ && app_->player()->GetState() == GstEngine::State::Playing;
  if (!PlaylistMenu::PlayEnabled(state)) {
    return;
  }
  if (state.playing_selected) {
    app_->player()->Pause();
  } else {
    app_->player()->PlayAt(playlist_menu_row_);
  }
}

void MainWindow::PlaylistStopAfter() {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist || playlist_menu_row_ < 0) {
    return;
  }
  playlist->ToggleStopAfter(playlist_menu_row_);
  app_->player()->set_stop_after_current(PlayerStopAfter::SyncCurrentFlag(playlist->stop_after_row(), playlist->current_row()));
  RefreshPlaylist();
  ShowToast(PlaylistStopAfter::IsRow(playlist->stop_after_row(), playlist_menu_row_) ? "Will stop after this track"
                                                                                     : "Stop after this track cancelled");
}

void MainWindow::QueuePlayNext() {
  Playlist *playlist = app_->playlist_manager()->current();
  const std::vector<int> rows = SelectedPlaylistRows();
  if (playlist && !rows.empty()) {
    for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
      if (*it >= 0 && *it < playlist->row_count()) {
        app_->queue()->InsertNext(playlist->song(*it), playlist->id(), *it);
      }
    }
  } else {
    for (const Song &song : SelectedSongs()) {
      app_->queue()->InsertNext(song);
    }
  }
  RefreshQueue();
  RefreshPlaylist();
  ShowToast("Queued to play next");
}

void MainWindow::ShowInCollection() {
  const SongList songs = SelectedSongs();
  if (songs.empty()) {
    return;
  }
  const std::string filter = CollectionBehaviour::ShowInCollectionQuery(songs);
  adw_view_stack_set_visible_child_name(sidebar_stack_, "collection");
  RefreshCollection(filter, true);
  if (!filter.empty()) {
    ShowToast("Showing “" + filter + "” in collection");
  }
}

void MainWindow::OpenSelectedInFileManager() {
  const SongList songs = SelectedSongs();
  if (songs.empty()) {
    ShowToast("No song selected");
    return;
  }
  const std::string path = FileUtils::PathFromUri(songs.front().url());
  if (path.empty() || !FileManagerUtils::OpenInFileManager(path)) {
    ShowToast("Could not open the file manager");
    return;
  }
}

SongList MainWindow::SongsFromFilePaths(const std::vector<std::string> &paths) const {
  return FileViewSongs::FromPaths(paths, [this](const std::string &path) { return app_->tagreader()->ReadFile(path); });
}

void MainWindow::CopyFileViewToCollection(const std::vector<std::string> &paths, bool move) {
  const std::vector<std::string> filenames = OrganizeLoading::FileViewFilenames(paths);
  if (filenames.empty()) {
    ShowToast("No files selected");
    return;
  }
  OrganizeDialog::Request request;
  request.filenames = filenames;
  request.move = move;
  OrganizeDialog::Show(GTK_WINDOW(window_), app_, request);
}

void MainWindow::CopySelectedToCollection(bool move) {
  SongList songs = SelectedSongs();
  if (songs.empty()) {
    ShowToast("No songs selected");
    return;
  }
  Dialogs::Organize(GTK_WINDOW(window_), app_, songs, move);
}

void MainWindow::AddSelectedToTranscoder() {
  TranscodeDialog::Show(GTK_WINDOW(window_), app_, SelectedSongs());
}

void MainWindow::RenumberTracks() {
  if (Playlist *playlist = app_->playlist_manager()->current()) {
    playlist->RenumberTracks();
    app_->playlist_manager()->SaveCurrent();
    RefreshPlaylist();
  }
}

void MainWindow::RemoveDuplicates() {
  app_->playlist_manager()->RemoveDuplicatesCurrent();
  RefreshPlaylist();
}

void MainWindow::RemoveUnavailable() {
  app_->playlist_manager()->RemoveUnavailableCurrent();
  RefreshPlaylist();
}

void MainWindow::ShuffleCurrent() {
  app_->playlist_manager()->ShuffleCurrent();
  RefreshPlaylist();
}

void MainWindow::RateSelected(int stars) {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return;
  }
  std::vector<int> rows = SelectedPlaylistRows();
  if (rows.empty() && playlist->current_row() >= 0) {
    rows.push_back(playlist->current_row());
  }
  RateRows(rows, static_cast<float>(std::clamp(stars, 0, 5)) / 5.0f);
}

void MainWindow::RateRow(int row, float rating) { RateRows({row}, rating); }

void MainWindow::RateRows(const std::vector<int> &rows, float rating) {
  if (PlaylistColumnLayout::RatingLocked()) {
    ShowToast(Translations::Tr("Rating is locked"));
    return;
  }
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return;
  }
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  const bool save_ratings = settings.BoolValue(CollectionSettings::kSaveRatings, CollectionSettings::kDefaultSaveRatings);
  for (int row : rows) {
    Song song = playlist->song(row);
    song.set_rating(rating);
    playlist->ReplaceRow(row, song);
    if (song.id() > 0) {
      app_->collection()->backend()->SetRating(song.id(), rating);
    }
    if (save_ratings && song.IsEditable()) {
      app_->collection()->SongsRatingChanged({song}, true);
    }
  }
  RefreshPlaylist();
}

void MainWindow::ScrobbleCurrent() {
  const Song song = app_->player()->current_song();
  if (!song.is_valid() && song.url().empty()) {
    ShowToast("Nothing to scrobble");
    return;
  }
  app_->scrobbler()->Scrobble(song);
  ShowToast("Scrobbled “" + song.PrettyTitleWithArtist() + "”");
}

void MainWindow::UpdateScrobblerButtons() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const bool show_scrobble = settings.BoolValue(ScrobblerSettings::kScrobbleButton, ScrobblerSettings::kDefaultScrobbleButton);
  const bool show_love = settings.BoolValue(ScrobblerSettings::kLoveButton, ScrobblerSettings::kDefaultLoveButton);
  const bool scrobbler_on = app_ && app_->scrobbler() && app_->scrobbler()->enabled();
  const Song song = app_ && app_->player() ? app_->player()->current_song() : Song();
  const bool love_enabled = ScrobblerLoveState::CanLove(scrobbler_on, song) && !loved_current_track_;
  if (scrobble_button_) {
    gtk_widget_set_visible(scrobble_button_, show_scrobble);
    gtk_button_set_icon_name(GTK_BUTTON(scrobble_button_), ScrobbleToggleIcon::Name(scrobbler_on));
  }
  if (love_button_) {
    gtk_widget_set_visible(love_button_, show_love);
    gtk_widget_set_sensitive(love_button_, love_enabled);
  }
  if (app_ && app_->tray()) {
    app_->tray()->SetLoveVisible(show_love);
    app_->tray()->SetLoveEnabled(love_enabled);
  }
}

void MainWindow::SkipSelected() {
  if (Playlist *playlist = app_->playlist_manager()->current()) {
    playlist->SkipTracks(SelectedPlaylistRows());
    app_->playlist_manager()->SaveCurrent();
    RefreshPlaylist();
  }
}

void MainWindow::JumpToPlaying() {
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist || !playlist_container_) {
    return;
  }
  app_->playlist_manager()->SetCurrentPlaylist(playlist->id());
  const int row = playlist->current_row();
  if (row >= 0) {
    SelectPlaylistRow(row, false);
    playlist_container_->view()->MaybeScrollToRow(row, Playlist::AutoScroll::Always);
  }
  RefreshPlaylist();
}

void MainWindow::RescanSelected() {
  const SongList songs = SelectedSongs();
  if (songs.empty()) {
    ShowToast("No songs selected");
    return;
  }
  app_->collection()->Rescan(songs);
  if (Playlist *playlist = app_->playlist_manager()->current()) {
    for (int row : SelectedPlaylistRows()) {
      playlist->ReloadRow(row, app_->tagreader());
    }
    app_->playlist_manager()->SaveCurrent();
  }
  RefreshCollection();
  RefreshPlaylist();
  ShowToast("Rescanned " + std::to_string(songs.size()) + " song(s)");
}

void MainWindow::FetchStreamingMetadata() {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return;
  }
  const std::vector<int> rows = SelectedPlaylistRows();
  SongList songs;
  std::vector<int> aligned;
  for (int row : rows) {
    if (row < 0 || row >= playlist->row_count()) {
      continue;
    }
    songs.push_back(playlist->song(row));
    aligned.push_back(row);
  }
  const std::vector<StreamingMetadataQueue::Entry> queued = StreamingMetadataQueue::EntriesFromSelection(songs, aligned);
  metadata_queue_.insert(metadata_queue_.end(), queued.begin(), queued.end());
  if (StreamingMetadataQueue::ShouldStart(metadata_queue_.empty(), metadata_queue_timeout_ != 0)) {
    ProcessMetadataQueue();
  }
  ShowToast(!queued.empty() ? ("Fetching metadata for " + std::to_string(queued.size()) + " stream(s)") : "No streaming tracks selected");
}

void MainWindow::ScheduleMetadataQueue() {
  if (metadata_queue_timeout_ != 0 || !StreamingMetadataQueue::ShouldContinue(metadata_queue_.empty())) {
    return;
  }
  metadata_queue_timeout_ = g_timeout_add(StreamingMetadataQueue::kDelayMs, [](gpointer data) -> gboolean {
    auto *self = static_cast<MainWindow *>(data);
    self->metadata_queue_timeout_ = 0;
    self->ProcessMetadataQueue();
    return G_SOURCE_REMOVE;
  }, this);
}

void MainWindow::ApplyFetchedMetadata(const StreamingMetadataQueue::Entry &entry, const Song &fetched, const std::string &error) {
  if (!error.empty()) {
    LogError("Failed to fetch streaming metadata: %s", error.c_str());
  }
  Playlist *playlist = app_->playlist_manager()->current();
  if (playlist && StreamingMetadataMerge::ShouldApply(fetched)) {
    const int row = EditTagSave::ResolveRow(playlist->songs(), entry.row, entry.url);
    if (row >= 0) {
      Song updated = playlist->song(row);
      StreamingMetadataMerge::Apply(&updated, fetched);
      playlist->ReplaceRow(row, updated);
      app_->playlist_manager()->SaveCurrent();
      RefreshPlaylist();
    }
  }
  ScheduleMetadataQueue();
}

void MainWindow::ProcessMetadataQueue() {
  if (metadata_queue_.empty()) {
    return;
  }
  const StreamingMetadataQueue::Entry entry = metadata_queue_.front();
  metadata_queue_.erase(metadata_queue_.begin());
  auto alive = metadata_alive_;
  auto finish = [this, entry, alive](const Song &fetched, const std::string &error) {
    if (!alive || !*alive) {
      return;
    }
    ApplyFetchedMetadata(entry, fetched, error);
  };

#ifdef HAVE_QOBUZ
  if (entry.source == Song::Source::Qobuz) {
    if (auto *qobuz = dynamic_cast<QobuzService *>(app_->streaming_services()->ServiceByName("Qobuz"))) {
      qobuz->FetchTrackMetadata(entry.track_id, finish);
      return;
    }
  }
#endif
#ifdef HAVE_SPOTIFY
  if (entry.source == Song::Source::Spotify) {
    if (auto *spotify = dynamic_cast<SpotifyService *>(app_->streaming_services()->ServiceByName("Spotify"))) {
      spotify->FetchTrackMetadata(entry.track_id, finish);
      return;
    }
  }
#endif
  ScheduleMetadataQueue();
}

void MainWindow::AddSelectedToPlaylist(int id) {
  const SongList songs = SelectedSongs();
  if (songs.empty()) {
    return;
  }
  if (id < 0) {
    Playlist *created = app_->playlist_manager()->New("Playlist", songs);
    RefreshPlaylistsList();
    RefreshPlaylistTabs();
    ShowToast("Added " + std::to_string(songs.size()) + " song(s) to " + (created ? created->name() : std::string("Playlist")));
    return;
  }
  app_->playlist_manager()->InsertSongs(id, songs);
  RefreshPlaylistsList();
  ShowToast("Added " + std::to_string(songs.size()) + " song(s) to " + app_->playlist_manager()->playlist_name(id));
}

void MainWindow::AutoCompleteTags() {
  TrackSelectionDialog::Show(GTK_WINDOW(window_), app_, SelectedSongs());
}

void MainWindow::FocusCollectionSearch() {
  if (collection_search_) {
    gtk_widget_grab_focus(collection_search_);
  }
}

void MainWindow::FocusSearchField() {
  const char *page = sidebar_stack_ ? adw_view_stack_get_visible_child_name(sidebar_stack_) : "";
  StreamingTabsView *tabs = CurrentStreamingTabs();
  const MainWindowSearchFocus::Target target = MainWindowSearchFocus::Resolve(
      page ? page : "", collection_search_ && gtk_widget_has_focus(collection_search_),
      tabs && tabs->SearchFieldHasFocus(), playlist_container_ && playlist_container_->SearchFieldHasFocus());
  switch (target) {
    case MainWindowSearchFocus::Target::Collection:
      FocusCollectionSearch();
      break;
    case MainWindowSearchFocus::Target::Streaming:
      if (tabs) {
        tabs->FocusSearchField();
      }
      break;
    case MainWindowSearchFocus::Target::Playlist:
      if (playlist_container_) {
        playlist_container_->FocusFilter();
      }
      break;
    case MainWindowSearchFocus::Target::None:
      break;
  }
}

void MainWindow::PersistEditedSongs(const std::vector<int> &rows) {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist) {
    return;
  }
  playlist->set_tagreader_client(app_->tagreader_client());
  playlist->SaveRows(rows);
  app_->playlist_manager()->ArmTagReaderPump();
}

void MainWindow::ApplyColumnValue(PlaylistColumn column, const std::string &value, const std::vector<int> &rows) {
  Playlist *playlist = app_->playlist_manager()->current();
  if (!playlist || rows.empty()) {
    return;
  }
  std::vector<int> editable_rows;
  for (int row : rows) {
    if (playlist->song(row).IsEditable()) {
      editable_rows.push_back(row);
    }
  }
  if (editable_rows.empty()) {
    ShowToast("Selected songs cannot be edited");
    return;
  }
  if (playlist->SetColumnValues(editable_rows, column, value) <= 0) {
    ShowToast("This column is not editable");
    return;
  }
  PersistEditedSongs(editable_rows);
  app_->playlist_manager()->SaveCurrent();
  RefreshPlaylist();
  UpdateNowPlaying();
  ShowToast("Updated " + PlaylistDelegates::ColumnTitle(column));
}

void MainWindow::EditColumnValue() {
  if (PlaylistEditPolicy::MenuEditRequiresInlineSetting()) {
    Settings settings;
    settings.BeginGroup(PlaylistSettings::kSettingsGroup);
    if (!settings.BoolValue(PlaylistSettings::kEditMetadataInline, PlaylistSettings::kDefaultEditMetadataInline)) {
      ShowToast("Enable “Edit metadata inline” in playlist preferences");
      return;
    }
  }
  Playlist *playlist = app_->playlist_manager()->current();
  const std::vector<int> rows = SelectedPlaylistRows();
  if (!playlist || rows.empty()) {
    return;
  }
  const PlaylistColumn column =
      PlaylistEditColumn::Resolve(playlist_container_ ? playlist_container_->view()->last_clicked_column() : PlaylistColumn::Count,
                                  PlaylistColumnLayout::Visible());
  if (!PlaylistDelegates::ColumnIsEditable(column)) {
    ShowToast("This column is not editable");
    return;
  }
  if (rows.size() == 1 && playlist_container_) {
    playlist_container_->view()->StartInlineEdit(rows.front(), column);
    return;
  }
  const std::string current = PlaylistDelegates::ColumnText(playlist->song(rows.front()), column);
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Edit value", PlaylistDelegates::ColumnTitle(column).c_str()));
  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), current.c_str());
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "apply", "Apply", nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "apply", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "apply");
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_object_set_data(G_OBJECT(dialog), "column", GINT_TO_POINTER(static_cast<int>(column) + 1));
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "apply") != 0) {
                       return;
                     }
                     auto *self = static_cast<MainWindow *>(data);
                     auto *value_entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry"));
                     const PlaylistColumn edited =
                         static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(alert), "column")) - 1);
                     self->ApplyColumnValue(edited, gtk_editable_get_text(value_entry), self->SelectedPlaylistRows());
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window_));
}

void MainWindow::SetColumnTo() {
  Playlist *playlist = app_->playlist_manager()->current();
  const std::vector<int> rows = SelectedPlaylistRows();
  if (!playlist || rows.empty()) {
    return;
  }
  const PlaylistColumn column =
      PlaylistEditColumn::Resolve(playlist_container_ ? playlist_container_->view()->last_clicked_column() : PlaylistColumn::Count,
                                  PlaylistColumnLayout::Visible());
  if (!PlaylistDelegates::ColumnIsEditable(column)) {
    ShowToast("This column is not editable");
    return;
  }
  const std::string value = PlaylistMenu::ClickedColumnValue(PlaylistDelegates::ColumnText(playlist->song(rows.front()), column));
  ApplyColumnValue(column, value, rows);
}

void MainWindow::CopySelectedUrl() {
  const std::vector<std::string> urls = PlaylistClipboard::EffectiveUrls(SelectedSongs());
  const std::string text = PlaylistClipboard::UrlsText(urls);
  if (text.empty()) {
    return;
  }
  gdk_clipboard_set_text(gtk_widget_get_clipboard(GTK_WIDGET(window_)), text.c_str());
  ShowToast(urls.size() == 1 ? "Copied URL" : ("Copied " + std::to_string(urls.size()) + " URLs"));
}

bool MainWindow::FocusIsEditable() const {
  GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(window_));
  return focus && GTK_IS_EDITABLE(focus);
}

gboolean MainWindow::OnWindowKeyCapture(guint keyval) {
  if (MainWindowKeyboard::FromWindowKey(keyval) != MainWindowKeyboard::Action::PlayPause) {
    return FALSE;
  }
  if (!MainWindowKeyboard::ShouldHandleWindowKey(FocusIsEditable())) {
    return FALSE;
  }
  app_->player()->PlayPause();
  return TRUE;
}

gboolean MainWindow::OnWindowKeyBubble(guint keyval) {
  const MainWindowKeyboard::Action action = MainWindowKeyboard::FromWindowKey(keyval);
  if (action == MainWindowKeyboard::Action::SeekBack) {
    app_->player()->SeekBackward();
    return TRUE;
  }
  if (action == MainWindowKeyboard::Action::SeekForward) {
    app_->player()->SeekForward();
    return TRUE;
  }
  return FALSE;
}

void MainWindow::OnPlayPause(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->PlayPause(); }
void MainWindow::OnStop(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Stop(); }
void MainWindow::OnNext(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Next(); }
void MainWindow::OnPrevious(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Previous(); }
void MainWindow::OnLove(GtkButton *, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  self->app_->scrobbler()->Love(self->app_->player()->current_song());
}

void MainWindow::DrawAnalyzer(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  self->app_->analyzer()->Draw(cr, width, height);
}

void MainWindow::ApplySeekbarPlaybackState() {
  if (!track_slider_) {
    return;
  }
  if (EngineStopped()) {
    track_slider_->SetStopped();
    return;
  }
  track_slider_->SetCanSeek(TrackSliderState::CanSeekFromSong(app_->player()->current_song()));
}

void MainWindow::ApplySeekbarFadeWidgets() {
  if (moodbar_drawing_) {
    gtk_widget_set_visible(moodbar_drawing_, moodbar_fade_.Visible());
    gtk_widget_set_opacity(moodbar_drawing_, moodbar_fade_.Opacity());
    gtk_widget_set_can_target(moodbar_drawing_, moodbar_fade_.Visible() && moodbar_fade_.Opacity() > 0.05F);
  }
  if (waveform_drawing_) {
    gtk_widget_set_visible(waveform_drawing_, waveform_fade_.Visible());
    gtk_widget_set_opacity(waveform_drawing_, waveform_fade_.Opacity());
    gtk_widget_set_can_target(waveform_drawing_, waveform_fade_.Visible() && waveform_fade_.Opacity() > 0.05F);
  }
  if (track_slider_) {
    const bool show_slider = SeekbarFade::ShowSlider(moodbar_fade_, waveform_fade_);
    gtk_widget_set_visible(track_slider_->slider()->widget(), TRUE);
    gtk_widget_set_opacity(track_slider_->slider()->widget(), show_slider ? 1.0 : 0.0);
    gtk_widget_set_can_target(track_slider_->slider()->widget(), show_slider);
  }
}

void MainWindow::StartSeekbarFadeTimer() {
  if (seekbar_fade_source_ || (!SeekbarFade::IsFading(moodbar_fade_.state) && !SeekbarFade::IsFading(waveform_fade_.state))) {
    return;
  }
  seekbar_fade_source_ = g_timeout_add(SeekbarFade::kTickMs, OnSeekbarFadeTick, this);
}

gboolean MainWindow::OnSeekbarFadeTick(gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  const bool mood_running = self->moodbar_fade_.Tick(SeekbarFade::kTickMs);
  const bool wave_running = self->waveform_fade_.Tick(SeekbarFade::kTickMs);
  self->ApplySeekbarFadeWidgets();
  if (!mood_running && !wave_running) {
    self->seekbar_fade_source_ = 0;
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

void MainWindow::ApplySeekbarMode() {
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  const auto mode = static_cast<SeekbarSettings::Mode>(
      settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode)));
  const bool show_moodbar = mode == SeekbarSettings::Mode::Moodbar;
  const bool show_waveform = mode == SeekbarSettings::Mode::Waveform;
  if (!seekbar_fade_inited_) {
    moodbar_fade_.Snap(show_moodbar);
    waveform_fade_.Snap(show_waveform);
    seekbar_fade_inited_ = true;
    ApplySeekbarFadeWidgets();
    return;
  }
  moodbar_fade_.Request(show_moodbar);
  waveform_fade_.Request(show_waveform);
  ApplySeekbarFadeWidgets();
  StartSeekbarFadeTimer();
}

void MainWindow::SetSeekbarMode(SeekbarSettings::Mode mode) {
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  settings.SetIntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarModeMenu::Clamp(static_cast<int>(mode))));
  settings.Sync();
  ApplySeekbarMode();
  app_->moodbar()->ReloadSettings();
  app_->waveform()->ReloadSettings();
}

void MainWindow::SetMoodbarStyle(MoodbarSettings::Style style) {
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  settings.SetIntValue(MoodbarSettings::kStyle, static_cast<int>(MoodbarStyle::ClampStyle(static_cast<int>(style))));
  settings.Sync();
  if (moodbar_drawing_) {
    gtk_widget_queue_draw(moodbar_drawing_);
  }
}

void MainWindow::CycleSeekbarMode() {
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  const auto current = static_cast<SeekbarSettings::Mode>(
      settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode)));
  SetSeekbarMode(SeekbarModeMenu::Next(current));
}

void MainWindow::ShowSeekbarMenu(GtkWidget *relative) {
  if (!relative) {
    return;
  }
  if (seekbar_menu_) {
    gtk_widget_unparent(seekbar_menu_);
    seekbar_menu_ = nullptr;
  }
  Settings settings;
  settings.BeginGroup(SeekbarSettings::kSettingsGroup);
  const auto current_mode = SeekbarModeMenu::Clamp(settings.IntValue(SeekbarSettings::kMode, static_cast<int>(SeekbarSettings::kDefaultMode)));
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  const auto current_style =
      MoodbarStyle::ClampStyle(settings.IntValue(MoodbarSettings::kStyle, static_cast<int>(MoodbarSettings::kDefaultStyle)));

  GMenu *menu = g_menu_new();
  GMenu *modes = g_menu_new();
  for (int i = 0; i < SeekbarModeMenu::kCount; ++i) {
    char action[32];
    g_snprintf(action, sizeof(action), "seekbar.mode(%d)", i);
    g_menu_append(modes, SeekbarModeMenu::Label(static_cast<SeekbarSettings::Mode>(i)), action);
  }
  g_menu_append_section(menu, nullptr, G_MENU_MODEL(modes));
  GMenu *styles = g_menu_new();
  for (int i = 0; i < static_cast<int>(MoodbarSettings::Style::StyleCount); ++i) {
    char action[32];
    g_snprintf(action, sizeof(action), "seekbar.style(%d)", i);
    g_menu_append(styles, MoodbarStyle::StyleName(static_cast<MoodbarSettings::Style>(i)), action);
  }
  g_menu_append_submenu(menu, SeekbarModeMenu::StyleSubmenuTitle(), G_MENU_MODEL(styles));

  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *mode_action = g_simple_action_new_stateful("mode", G_VARIANT_TYPE_INT32, g_variant_new_int32(static_cast<gint32>(current_mode)));
  GSimpleAction *style_action =
      g_simple_action_new_stateful("style", G_VARIANT_TYPE_INT32, g_variant_new_int32(static_cast<gint32>(current_style)));
  g_simple_action_set_enabled(style_action, SeekbarModeMenu::StyleMenuEnabled(current_mode) ? TRUE : FALSE);
  g_object_set_data(G_OBJECT(mode_action), "style-action", style_action);
  g_signal_connect(mode_action, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     g_simple_action_set_state(action, param);
                     const auto mode = SeekbarModeMenu::Clamp(g_variant_get_int32(param));
                     static_cast<MainWindow *>(data)->SetSeekbarMode(mode);
                     if (auto *style = static_cast<GSimpleAction *>(g_object_get_data(G_OBJECT(action), "style-action"))) {
                       g_simple_action_set_enabled(style, SeekbarModeMenu::StyleMenuEnabled(mode) ? TRUE : FALSE);
                     }
                   }),
                   this);
  g_signal_connect(style_action, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     g_simple_action_set_state(action, param);
                     static_cast<MainWindow *>(data)->SetMoodbarStyle(MoodbarStyle::ClampStyle(g_variant_get_int32(param)));
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(mode_action));
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(style_action));
  seekbar_menu_ = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_insert_action_group(seekbar_menu_, "seekbar", G_ACTION_GROUP(group));
  gtk_widget_set_parent(seekbar_menu_, relative);
  gtk_popover_popup(GTK_POPOVER(seekbar_menu_));
  g_object_unref(group);
  g_object_unref(modes);
  g_object_unref(styles);
  g_object_unref(menu);
}

void MainWindow::OnSeekbarScroll(double dy) {
  if (!track_slider_ || !TrackSliderState::ShouldAcceptSeek(track_slider_->stopped(), track_slider_->can_seek())) {
    return;
  }
  const TrackSliderWheel::Result result = TrackSliderWheel::FromGtkScroll(seekbar_wheel_accum_, dy);
  seekbar_wheel_accum_ = result.accumulator;
  const TrackSliderWheel::Direction direction = TrackSliderWheel::DirectionFromSteps(result.steps);
  if (direction == TrackSliderWheel::Direction::Forward) {
    app_->player()->SeekForward();
  } else if (direction == TrackSliderWheel::Direction::Backward) {
    app_->player()->SeekBackward();
  }
}

void MainWindow::SeekFromBar(double x, int width) {
  if (width <= 0 || !app_->player()->engine() || !track_slider_ ||
      !TrackSliderState::ShouldAcceptSeek(track_slider_->stopped(), track_slider_->can_seek())) {
    return;
  }
  const int64_t length = app_->player()->engine()->length_nanosec();
  if (length <= 0) {
    return;
  }
  const double ratio = std::clamp(x / static_cast<double>(width), 0.0, 1.0);
  app_->player()->Seek(static_cast<int64_t>(ratio * static_cast<double>(length)));
  if (moodbar_drawing_ && gtk_widget_get_visible(moodbar_drawing_)) {
    gtk_widget_queue_draw(moodbar_drawing_);
  }
  if (waveform_drawing_ && gtk_widget_get_visible(waveform_drawing_)) {
    gtk_widget_queue_draw(waveform_drawing_);
  }
}

void MainWindow::DrawMoodbar(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  int64_t pos = 0;
  int64_t len = 0;
  if (self->app_->player() && self->app_->player()->engine()) {
    pos = self->app_->player()->engine()->position_nanosec();
    len = self->app_->player()->engine()->length_nanosec();
  }
  const gboolean dark = adw_style_manager_get_dark(adw_style_manager_get_default());
  const double fill = dark ? 0.18 : 1.0;
  (void)area;
  MoodbarRenderer::Draw(cr, width, height, self->app_->moodbar()->data(), pos, len, fill, fill, fill);
}

void MainWindow::DrawWaveform(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  int64_t pos = 0;
  int64_t len = 0;
  if (self->app_->player() && self->app_->player()->engine()) {
    pos = self->app_->player()->engine()->position_nanosec();
    len = self->app_->player()->engine()->length_nanosec();
  }
  GdkRGBA cursor;
  gtk_widget_get_color(GTK_WIDGET(area), &cursor);
  WaveformRenderer::Draw(cr, width, height, self->app_->waveform()->data(), pos, len, cursor.red, cursor.green, cursor.blue);
}
