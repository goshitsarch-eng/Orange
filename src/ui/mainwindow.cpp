#include "ui/mainwindow.h"

#include "collection/collectionfilteroptions.h"
#include "collection/collectiongrouping.h"
#include "collection/collectionviewcontainer.h"
#include "context/contextview.h"
#include "fileview/fileview.h"
#include "playlist/playlistcontainer.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistlistcontainer.h"
#include "playlist/playlistsequence.h"
#include "widgets/multiloadingindicator.h"
#include "widgets/playingwidget.h"
#include "widgets/trackslider.h"
#include "widgets/volumeslider.h"
#include "core/settings.h"
#include "playlistparsers/playlistparser.h"
#include "smartplaylists/smartplaylist.h"
#include "streaming/streamingtabsview.h"
#include "ui/dialogs.h"
#include "ui/settingsdialog.h"
#include "filterparser/filterparser.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "version.h"

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
  app_->ApplyCommandline(options);
}

MainWindow::~MainWindow() {
  if (position_timeout_) {
    g_source_remove(position_timeout_);
  }
}

void MainWindow::Present() { gtk_window_present(GTK_WINDOW(window_)); }

void MainWindow::CommandlineReceived(const CommandlineOptions &options) {
  app_->ApplyCommandline(options);
  RefreshPlaylist();
  Present();
}

void MainWindow::BuildUi() {
  window_ = ADW_APPLICATION_WINDOW(adw_application_window_new(GTK_APPLICATION(gtk_app_)));
  gtk_window_set_title(GTK_WINDOW(window_), "Strawberry");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1400, 860);

  GtkWidget *header = adw_header_bar_new();
  GtkWidget *menu_button = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
  GMenu *menu = g_menu_new();
  GMenu *music = g_menu_new();
  g_menu_append(music, "Open files…", "win.open-files");
  g_menu_append(music, "Add collection folder…", "win.add-folder");
  g_menu_append(music, "Add stream…", "win.add-stream");
  g_menu_append_section(menu, "Music", G_MENU_MODEL(music));
  GMenu *playlist = g_menu_new();
  g_menu_append(playlist, "New playlist", "win.new-playlist");
  g_menu_append(playlist, "Load playlist…", "win.load-playlist");
  g_menu_append(playlist, "Save playlist…", "win.save-playlist");
  g_menu_append(playlist, "Save all playlists…", "win.save-all-playlists");
  g_menu_append(playlist, "Playlist columns…", "win.playlist-columns");
  g_menu_append(playlist, "Clear playlist", "win.clear-playlist");
  g_menu_append(playlist, "Undo", "win.undo");
  g_menu_append(playlist, "Redo", "win.redo");
  g_menu_append(playlist, "Smart playlist wizard…", "win.smart-wizard");
  g_menu_append_section(menu, "Playlist", G_MENU_MODEL(playlist));
  GMenu *tools = g_menu_new();
  g_menu_append(tools, "Cover manager", "win.covers");
  g_menu_append(tools, "Cover search…", "win.cover-search");
  g_menu_append(tools, "Cover from URL…", "win.cover-from-url");
  g_menu_append(tools, "Export covers…", "win.cover-export");
  g_menu_append(tools, "Equalizer", "win.equalizer");
  g_menu_append(tools, "Transcode…", "win.transcode");
  g_menu_append(tools, "Organize files…", "win.organize");
  g_menu_append(tools, "Copy to device…", "win.copy-device");
  g_menu_append(tools, "Delete files…", "win.delete-files");
  g_menu_append(tools, "Fetch tags…", "win.tagfetch");
  g_menu_append(tools, "Edit tags…", "win.edittag");
  g_menu_append(tools, "Collection grouping…", "win.group-by");
  g_menu_append(tools, "Cycle analyzer", "win.cycle-analyzer");
  g_menu_append(tools, "Debug console", "win.console");
  g_menu_append_section(menu, "Tools", G_MENU_MODEL(tools));
  GMenu *appmenu = g_menu_new();
  g_menu_append(appmenu, "Preferences", "win.preferences");
  g_menu_append(appmenu, "Keyboard shortcuts", "win.shortcuts");
  g_menu_append(appmenu, "About Strawberry", "win.about");
  g_menu_append(appmenu, "Quit", "win.quit");
  g_menu_append_section(menu, nullptr, G_MENU_MODEL(appmenu));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));
  adw_header_bar_pack_end(ADW_HEADER_BAR(header), menu_button);

  GtkWidget *search = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search), "Filter collection (artist:name, -term)");
  gtk_widget_set_tooltip_text(search, "Field filters: artist: title: album: genre: year: rating:  Use -term to exclude.");
  adw_header_bar_pack_start(ADW_HEADER_BAR(header), search);
  g_signal_connect(search, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     self->RefreshCollection(gtk_editable_get_text(GTK_EDITABLE(entry)), true);
                   }),
                   this);

  GtkWidget *toolbar = adw_toolbar_view_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);

  toast_overlay_ = ADW_TOAST_OVERLAY(adw_toast_overlay_new());
  GtkWidget *split = adw_overlay_split_view_new();
  adw_overlay_split_view_set_sidebar_width_fraction(ADW_OVERLAY_SPLIT_VIEW(split), 0.30);

  BuildSidebar();
  GtkWidget *sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_policy(ADW_VIEW_SWITCHER(switcher), ADW_VIEW_SWITCHER_POLICY_NARROW);
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), sidebar_stack_);
  gtk_box_append(GTK_BOX(sidebar_box), switcher);
  gtk_box_append(GTK_BOX(sidebar_box), GTK_WIDGET(sidebar_stack_));
  gtk_widget_set_vexpand(GTK_WIDGET(sidebar_stack_), TRUE);
  adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(split), sidebar_box);

  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  BuildPlaylist();
  gtk_box_append(GTK_BOX(content), playlist_container_->widget());
  BuildPlayerBar();
  gtk_box_append(GTK_BOX(content), GTK_WIDGET(g_object_get_data(G_OBJECT(play_button_), "player-box")));
  adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(split), content);

  adw_toast_overlay_set_child(toast_overlay_, split);
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(toast_overlay_));
  adw_application_window_set_content(window_, toolbar);

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
  add_action("new-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->NewPlaylist(); }));
  add_action("load-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->LoadPlaylistFile(); }));
  add_action("save-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->SavePlaylistFile(); }));
  add_action("clear-playlist", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->ClearPlaylist(); }));
  add_action("add-stream", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::AddStream(GTK_WINDOW(self->window_), [self](const std::string &name, const std::string &url) {
                 self->app_->radio_services()->AddCustomStream(name, url);
                 self->RefreshRadio();
               });
             }));
  add_action("covers", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverManager(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("cover-search", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverSearch(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("cover-from-url", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverFromUrl(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("cover-export", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverExport(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("copy-device", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CopyToDevice(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("delete-files", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::DeleteFiles(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("save-all-playlists", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::SaveAllPlaylists(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("playlist-columns", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::PlaylistColumns(GTK_WINDOW(self->window_), [self]() { self->RefreshPlaylist(); });
             }));
  add_action("equalizer", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Equalizer(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_->equalizer()); }));
  add_action("transcode", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Transcode(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("organize", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Organize(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("tagfetch", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::TagFetcher(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("edittag", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::EditTag(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
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
                 self->RefreshCollection();
               });
             }));
  add_action("console", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Console(GTK_WINDOW(static_cast<MainWindow *>(data)->window_)); }));
  add_action("cycle-analyzer", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { static_cast<MainWindow *>(data)->CycleAnalyzer(); }));
  add_action("playlist-play", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (self->app_->playlist_manager()->active() && self->app_->playlist_manager()->current_row() >= 0) {
                 self->app_->player()->PlayAt(self->app_->playlist_manager()->current_row());
               } else {
                 self->app_->player()->Play();
               }
             }));
  add_action("playlist-queue", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Playlist *playlist = self->app_->playlist_manager()->active();
               if (!playlist) {
                 return;
               }
               for (int row : self->SelectedPlaylistRows()) {
                 if (row >= 0 && row < playlist->row_count()) {
                   self->app_->queue()->Append(playlist->songs()[static_cast<size_t>(row)]);
                 }
               }
               self->RefreshQueue();
             }));
  add_action("playlist-remove", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (Playlist *playlist = self->app_->playlist_manager()->active()) {
                 playlist->RemoveRows(self->SelectedPlaylistRows());
                 self->selected_playlist_rows_.clear();
                 self->app_->playlist_manager()->SaveActive();
                 self->RefreshPlaylist();
               }
             }));
}

void MainWindow::BuildSidebar() {
  sidebar_stack_ = ADW_VIEW_STACK(adw_view_stack_new());
  gtk_widget_set_vexpand(GTK_WIDGET(sidebar_stack_), TRUE);

  BuildContext();
  adw_view_stack_add_titled_with_icon(sidebar_stack_, context_view_->widget(), "context", "Context", "audio-x-generic-symbolic");
  GtkWidget *collection_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  collection_container_ = std::make_unique<CollectionViewContainer>();
  gtk_widget_set_vexpand(collection_container_->widget(), TRUE);
  collection_container_->filter_widget()->SetChangedCallback([this]() { RefreshCollection(); });
  collection_container_->view()->SetActivateCallback([this](const SongList &songs) {
    app_->playlist_manager()->AppendSongs(songs);
    RefreshPlaylist();
  });
  gtk_box_append(GTK_BOX(collection_page), collection_container_->widget());
  adw_view_stack_add_titled_with_icon(sidebar_stack_, collection_page, "collection", "Collection",
                                      "media-optical-cd-audio-symbolic");
  playlist_list_container_ = std::make_unique<PlaylistListContainer>();
  playlist_list_container_->SetActivateCallback([this](const std::string &name) {
    app_->playlist_manager()->SetCurrentPlaylist(name);
    RefreshPlaylist();
    RefreshPlaylistTabs();
  });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, playlist_list_container_->widget(), "playlists", "Playlists",
                                      "view-list-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&smart_list_), "smart", "Smart playlists", "view-refresh-symbolic");
  file_view_ = std::make_unique<FileView>();
  file_view_->SetAddToPlaylistCallback([this](const std::vector<std::string> &paths) {
    std::vector<std::string> urls;
    urls.reserve(paths.size());
    for (const std::string &path : paths) {
      urls.push_back(FileUtils::UriFromPath(path));
    }
    app_->playlist_manager()->InsertUrls(urls);
    RefreshPlaylist();
  });
  file_view_->SetCopyToCollectionCallback([this](const std::vector<std::string> &paths) {
    for (const std::string &path : paths) {
      if (FileUtils::IsDirectory(path)) {
        app_->collection()->AddDirectory(path, true);
        continue;
      }
      const auto dirs = app_->collection()->backend()->Directories();
      if (!dirs.empty()) {
        FileUtils::CopyFile(path, FileUtils::Join(dirs.front().path, FileUtils::BaseName(path)));
      } else {
        app_->collection()->AddDirectory(FileUtils::DirName(path), false);
      }
    }
    RefreshCollection();
  });
  file_view_->SetCopyToDeviceCallback([this](const std::vector<std::string> &) {
    Dialogs::CopyToDevice(GTK_WINDOW(window_), app_);
  });
  file_view_->SetEditTagsCallback([this](const std::vector<std::string> &paths) {
    if (!paths.empty()) {
      app_->playlist_manager()->InsertUrls({FileUtils::UriFromPath(paths.front())});
      RefreshPlaylist();
    }
    Dialogs::EditTag(GTK_WINDOW(window_), app_);
  });
  file_view_->SetDeleteCallback([this](const std::vector<std::string> &paths) {
    for (const std::string &path : paths) {
      FileUtils::Remove(path);
    }
    if (file_view_) {
      file_view_->Reload();
    }
  });
  adw_view_stack_add_titled_with_icon(sidebar_stack_, file_view_->widget(), "files", "Files", "folder-symbolic");
  GtkWidget *radio_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *radio_search = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(radio_search), "Search Radio Browser");
  gtk_widget_set_margin_start(radio_search, 8);
  gtk_widget_set_margin_end(radio_search, 8);
  gtk_widget_set_margin_top(radio_search, 6);
  gtk_widget_set_margin_bottom(radio_search, 4);
  gtk_box_append(GTK_BOX(radio_page), radio_search);
  gtk_box_append(GTK_BOX(radio_page), MakeScrolledList(&radio_list_));
  gtk_widget_set_vexpand(gtk_widget_get_last_child(radio_page), TRUE);
  g_signal_connect(radio_search, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->SearchRadio(text ? text : "");
                   }),
                   this);
  adw_view_stack_add_titled_with_icon(sidebar_stack_, radio_page, "radio", "Internet radio", "network-wireless-symbolic");
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
    auto view = std::make_unique<StreamingTabsView>(service);
    view->SetActivateCallback(activate_stream);
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
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&devices_list_), "devices", "Devices", "drive-harddisk-usb-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&queue_list_), "queue", "Queue", "view-list-ordered-symbolic");

  g_signal_connect(smart_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *kind = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (kind && std::string(kind) == "wizard") {
                       Dialogs::SmartPlaylistWizard(GTK_WINDOW(self->window_), self->app_);
                       self->RefreshPlaylistsList();
                       self->RefreshPlaylist();
                     } else if (kind) {
                       self->RunSmartPlaylist(kind);
                     }
                   }),
                   this);
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
  g_signal_connect(radio_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     auto *channel = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (channel) {
                       self->PlayRadioChannel(*channel);
                     }
                   }),
                   this);
  g_signal_connect(devices_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *kind = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-kind"));
                     if (kind && std::string(kind) == "back") {
                       self->device_browse_id_.clear();
                       self->RefreshDevices();
                       return;
                     }
                     if (kind && std::string(kind) == "add-all" && !self->device_browse_id_.empty()) {
                       self->app_->playlist_manager()->AppendSongs(self->app_->device_manager()->Songs(self->device_browse_id_));
                       self->RefreshPlaylist();
                       return;
                     }
                     if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"))) {
                       if (kind && std::string(kind) == "song") {
                         self->app_->playlist_manager()->AppendSongs({*song});
                         self->RefreshPlaylist();
                         if (self->app_->playlist_manager()->active()) {
                           self->app_->player()->PlayAt(self->app_->playlist_manager()->active()->row_count() - 1);
                         }
                         return;
                       }
                     }
                     const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (id && *id) {
                       self->device_browse_id_ = id;
                       self->RefreshDevices();
                     }
                   }),
                   this);
  g_signal_connect(queue_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index"));
                     SongList songs = self->app_->queue()->songs();
                     if (index >= 0 && index < static_cast<int>(songs.size())) {
                       self->app_->playlist_manager()->AppendSongs({songs[static_cast<size_t>(index)]});
                       self->app_->player()->PlayAt(self->app_->playlist_manager()->active()->row_count() - 1);
                     }
                   }),
                   this);
}

void MainWindow::BuildContext() {
  context_view_ = std::make_unique<ContextView>(app_->lyrics_providers());
  context_view_->SetSaveLyricsCallback([this](const std::string &lyrics) {
    Song song = app_->player()->current_song();
    song.set_lyrics(lyrics);
    if (app_->tagreader()->WriteFile(song) && song.id() > 0) {
      app_->collection()->backend()->AddOrUpdateSong(song);
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
    RefreshPlaylist();
  });
  playlist_container_->view()->SetActivateCallback([this](int index) { app_->player()->PlayAt(index); });
  playlist_container_->view()->SetSelectCallback([this](int index, bool add) { SelectPlaylistRow(index, add); });
  playlist_container_->view()->SetSortCallback([this](PlaylistColumn column) { SortPlaylistBy(column); });
  playlist_container_->view()->SetMenuCallback([this](double x, double y) { ShowPlaylistMenu(x, y); });
  playlist_container_->tab_bar()->SetChangedCallback([this](const std::string &name) {
    app_->playlist_manager()->SetCurrentPlaylist(name);
    RefreshPlaylist();
    RefreshPlaylistTabs();
  });
  playlist_container_->tab_bar()->SetFavoriteCallback([this](const std::string &name, bool on) {
    for (const auto &playlist : app_->playlist_manager()->playlists()) {
      if (playlist->name() == name) {
        playlist->set_favorite(on);
        app_->playlist_manager()->SaveActive();
        break;
      }
    }
  });

  repeat_button_ = playlist_container_->repeat_button();
  shuffle_button_ = playlist_container_->shuffle_button();
  g_signal_connect(repeat_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<MainWindow *>(data)->CycleRepeat();
                   }),
                   this);
  g_signal_connect(shuffle_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<MainWindow *>(data)->CycleShuffle();
                   }),
                   this);
}

void MainWindow::BuildPlayerBar() {
  playing_widget_ = std::make_unique<PlayingWidget>();
  track_slider_ = std::make_unique<TrackSlider>();
  volume_slider_ = std::make_unique<VolumeSlider>();
  track_slider_->SetSeekCallback([this](int64_t pos) { app_->player()->Seek(pos); });
  volume_slider_->SetVolumeCallback([this](unsigned volume) { app_->player()->SetVolume(volume); });
  volume_slider_->SetVolume(app_->player()->GetVolume());

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  gtk_box_append(GTK_BOX(box), playing_widget_->widget());

  waveform_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(waveform_drawing_, -1, 28);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(waveform_drawing_), DrawWaveform, this, nullptr);
  gtk_box_append(GTK_BOX(box), waveform_drawing_);
  moodbar_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(moodbar_drawing_, -1, 16);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(moodbar_drawing_), DrawMoodbar, this, nullptr);
  gtk_box_append(GTK_BOX(box), moodbar_drawing_);
  gtk_box_append(GTK_BOX(box), track_slider_->widget());

  GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(controls, GTK_ALIGN_CENTER);
  GtkWidget *prev = gtk_button_new_from_icon_name("media-skip-backward-symbolic");
  play_button_ = gtk_button_new_from_icon_name("media-playback-start-symbolic");
  gtk_widget_add_css_class(play_button_, "suggested-action");
  gtk_widget_add_css_class(play_button_, "circular");
  GtkWidget *stop = gtk_button_new_from_icon_name("media-playback-stop-symbolic");
  GtkWidget *next = gtk_button_new_from_icon_name("media-skip-forward-symbolic");
  GtkWidget *love = gtk_button_new_from_icon_name("emblem-favorite-symbolic");
  analyzer_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(analyzer_drawing_, 160, 36);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(analyzer_drawing_), DrawAnalyzer, this, nullptr);
  gtk_box_append(GTK_BOX(controls), prev);
  gtk_box_append(GTK_BOX(controls), play_button_);
  gtk_box_append(GTK_BOX(controls), stop);
  gtk_box_append(GTK_BOX(controls), next);
  gtk_box_append(GTK_BOX(controls), love);
  gtk_widget_set_tooltip_text(analyzer_drawing_, "Click to cycle analyzer type");
  GtkGesture *analyzer_click = gtk_gesture_click_new();
  gtk_widget_add_controller(analyzer_drawing_, GTK_EVENT_CONTROLLER(analyzer_click));
  g_signal_connect(analyzer_click, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble, gdouble, gpointer data) {
                     static_cast<MainWindow *>(data)->CycleAnalyzer();
                   }),
                   this);
  gtk_box_append(GTK_BOX(controls), analyzer_drawing_);
  gtk_box_append(GTK_BOX(controls), volume_slider_->widget());
  gtk_box_append(GTK_BOX(box), controls);

  loading_indicator_ = std::make_unique<MultiLoadingIndicator>();
  gtk_box_append(GTK_BOX(box), loading_indicator_->widget());
  status_label_ = gtk_label_new("Ready");
  gtk_widget_add_css_class(status_label_, "dim-label");
  gtk_box_append(GTK_BOX(box), status_label_);

  g_signal_connect(play_button_, "clicked", G_CALLBACK(OnPlayPause), this);
  g_signal_connect(stop, "clicked", G_CALLBACK(OnStop), this);
  g_signal_connect(next, "clicked", G_CALLBACK(OnNext), this);
  g_signal_connect(prev, "clicked", G_CALLBACK(OnPrevious), this);
  g_signal_connect(love, "clicked", G_CALLBACK(OnLove), this);
  g_object_set_data(G_OBJECT(play_button_), "player-box", box);
}

void MainWindow::ConnectSignals() {
  app_->player()->SongChanged.Connect([this](const Song &) { UpdateNowPlaying(); });
  app_->player()->StateChanged.Connect([this](GstEngine::State) { UpdatePlaybackButtons(); });
  app_->player()->VolumeChanged.Connect([this](unsigned volume) {
    if (volume_slider_) {
      volume_slider_->SetVolume(volume);
    }
  });
  app_->playlist_manager()->PlaylistsLoaded.Connect([this]() {
    RefreshPlaylistsList();
    RefreshPlaylist();
  });
  app_->collection()->ScanFinished.Connect([this]() { RefreshCollection(); });
  app_->task_manager()->TasksChanged.Connect([this](int) {
    if (!loading_indicator_) {
      return;
    }
    std::vector<std::string> names;
    for (const auto &task : app_->task_manager()->GetTasks()) {
      names.push_back(task.name);
    }
    loading_indicator_->SetTasks(names);
  });
  app_->current_albumcover_loader()->AlbumCoverReady.Connect([this](const Song &, const std::vector<unsigned char> &data) {
    if (!data.empty()) {
      UpdateCover(data);
    }
  });
  app_->queue()->Changed.Connect([this]() { RefreshQueue(); });
  app_->radio_services()->set_updated_callback([this]() { RefreshRadio(); });
  app_->waveform()->Ready.Connect([this](const std::vector<float> &) { gtk_widget_queue_draw(waveform_drawing_); });
  app_->moodbar()->Ready.Connect([this](const std::vector<uint8_t> &) { gtk_widget_queue_draw(moodbar_drawing_); });
  app_->player()->engine()->ScopeUpdated.Connect([this](const std::vector<int16_t> &scope) {
    app_->analyzer()->SetEngineScope(scope);
    gtk_widget_queue_draw(analyzer_drawing_);
  });
  position_timeout_ = g_timeout_add(200, [](gpointer data) -> gboolean {
    auto *self = static_cast<MainWindow *>(data);
    if (!self->app_->player()->engine()) {
      return G_SOURCE_CONTINUE;
    }
    const int64_t pos = self->app_->player()->engine()->position_nanosec();
    const int64_t len = self->app_->player()->engine()->length_nanosec();
    if (self->track_slider_) {
      self->track_slider_->SetTimes(pos, len);
    }
    return G_SOURCE_CONTINUE;
  }, this);
}

void MainWindow::RefreshCollection(const std::string &filter, bool update_text) {
  if (update_text) {
    collection_text_filter_ = filter;
  }
  if (!collection_container_) {
    return;
  }
  CollectionFilterOptions options = collection_container_->filter_widget()->options();
  SongList songs = app_->collection()->Songs(options);
  if (collection_container_->filter_widget()->unrated_only()) {
    songs.erase(std::remove_if(songs.begin(), songs.end(), [](const Song &song) { return song.rating() >= 0; }), songs.end());
  }
  Settings settings;
  settings.BeginGroup("Collection");
  collection_container_->view()->SetModelSongs(
      songs, grouping_, CollectionGrouping::SeparateAlbumsByGrouping(),
      settings.BoolValue("skip_articles_for_artists", settings.BoolValue("sort_skip_articles_for_artists", true)),
      settings.BoolValue("skip_articles_for_albums", settings.BoolValue("sort_skip_articles_for_albums", false)));
  collection_container_->view()->SetFilterString(collection_text_filter_);
  if (status_label_) {
    gtk_label_set_text(GTK_LABEL(status_label_), (std::to_string(collection_container_->view()->model()->TotalSongs()) + " songs").c_str());
  }
}

void MainWindow::SortPlaylistBy(PlaylistColumn column) {
  if (sort_column_ == column) {
    sort_descending_ = !sort_descending_;
  } else {
    sort_column_ = column;
    sort_descending_ = false;
  }
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist) {
    return;
  }
  SongList songs = playlist->songs();
  std::stable_sort(songs.begin(), songs.end(), [this](const Song &a, const Song &b) {
    const std::string left = PlaylistDelegates::ColumnText(a, sort_column_);
    const std::string right = PlaylistDelegates::ColumnText(b, sort_column_);
    if (sort_column_ == PlaylistColumn::Track || sort_column_ == PlaylistColumn::Year || sort_column_ == PlaylistColumn::OriginalYear ||
        sort_column_ == PlaylistColumn::Disc || sort_column_ == PlaylistColumn::Bitrate || sort_column_ == PlaylistColumn::Samplerate ||
        sort_column_ == PlaylistColumn::Bitdepth || sort_column_ == PlaylistColumn::PlayCount || sort_column_ == PlaylistColumn::SkipCount ||
        sort_column_ == PlaylistColumn::Length || sort_column_ == PlaylistColumn::Filesize || sort_column_ == PlaylistColumn::BPM) {
      const double ln = std::strtod(left.c_str(), nullptr);
      const double rn = std::strtod(right.c_str(), nullptr);
      return sort_descending_ ? ln > rn : ln < rn;
    }
    return sort_descending_ ? left > right : left < right;
  });
  playlist->ReplaceSongs(songs);
  app_->playlist_manager()->SaveActive();
  RefreshPlaylist();
}

void MainWindow::SelectPlaylistRow(int index, bool add) {
  Playlist *playlist = app_->playlist_manager()->active();
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
}

void MainWindow::RefreshPlaylist() {
  if (!playlist_container_) {
    return;
  }
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist || playlist->name() != selection_playlist_name_) {
    selected_playlist_rows_.clear();
    selection_playlist_name_ = playlist ? playlist->name() : std::string();
  }
  playlist_container_->view()->SetFilterString(playlist_filter_);
  playlist_container_->view()->SetSelectedRows(selected_playlist_rows_);
  playlist_container_->view()->Refresh(playlist);
  if (playlist) {
    playlist_container_->SetSummary(playlist->name() + " · " +
                                    std::to_string(playlist_container_->view()->visible_count()) + " tracks · " +
                                    Utilities::PrettyTimeNanosec(playlist->total_length_nanosec()));
  } else {
    playlist_container_->SetSummary("");
  }
}

void MainWindow::RefreshPlaylistsList() {
  if (playlist_list_container_) {
    playlist_list_container_->Reload(app_->playlist_manager());
  }
  RefreshPlaylistTabs();
}

void MainWindow::RefreshPlaylistTabs() {
  if (playlist_container_) {
    playlist_container_->tab_bar()->Refresh(app_->playlist_manager());
  }
}

void MainWindow::RefreshSmartPlaylists() {
  ClearList(smart_list_);
  struct Item {
    const char *title;
    const char *kind;
  };
  for (const Item item : {Item{"All songs", "all"}, Item{"Never played", "never"}, Item{"Highest rated", "rated"}, Item{"Newest", "newest"},
                          Item{"Most played", "played"}}) {
      AppendStringRow(GTK_LIST_BOX(smart_list_), item.title, const_cast<char *>(item.kind));
  }
  for (const auto &preset : SmartPlaylistSearch::LoadSaved()) {
    AppendStringRow(GTK_LIST_BOX(smart_list_), preset.first, g_strdup(("saved:" + preset.first).c_str()), g_free);
  }
  AppendStringRow(GTK_LIST_BOX(smart_list_), "Custom wizard…", const_cast<char *>("wizard"));
}

void MainWindow::RefreshQueue() {
  ClearList(queue_list_);
  int index = 0;
  for (const Song &song : app_->queue()->songs()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(song.PrettyTitleWithArtist().c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index++));
    gtk_list_box_append(GTK_LIST_BOX(queue_list_), row);
  }
  if (app_->queue()->empty()) {
    AppendStringRow(GTK_LIST_BOX(queue_list_), "Queue is empty", nullptr);
  }
}

void MainWindow::RefreshRadio() {
  ClearList(radio_list_);
  if (!radio_query_.empty()) {
    const std::vector<RadioChannel> &results = app_->radio_services()->search_results();
    for (const RadioChannel &channel : results) {
      std::string label = channel.name.empty() ? channel.url : channel.name;
      if (!channel.country.empty()) {
        label += " · " + channel.country;
      }
      if (!channel.codec.empty()) {
        label += " (" + channel.codec + ")";
      }
      auto *copy = new RadioChannel(channel);
      AppendStringRow(GTK_LIST_BOX(radio_list_), label, copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
    }
    if (results.empty()) {
      AppendStringRow(GTK_LIST_BOX(radio_list_), "Searching Radio Browser…", nullptr);
    }
    return;
  }
  if (app_->radio_services()->channels().empty()) {
    app_->radio_services()->FetchSomaFM();
    app_->radio_services()->FetchRadioParadise();
  }
  for (const RadioChannel &channel : app_->radio_services()->channels()) {
    auto *copy = new RadioChannel(channel);
    AppendStringRow(GTK_LIST_BOX(radio_list_), channel.name.empty() ? channel.url : channel.name, copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
  }
  if (app_->radio_services()->channels().empty()) {
    AppendStringRow(GTK_LIST_BOX(radio_list_), "SomaFM", nullptr);
    AppendStringRow(GTK_LIST_BOX(radio_list_), "Radio Paradise", nullptr);
    AppendStringRow(GTK_LIST_BOX(radio_list_), "Search Radio Browser above", nullptr);
  }
}

void MainWindow::SearchRadio(const std::string &query) {
  radio_query_ = StrUtils::Trim(query);
  if (radio_query_.empty()) {
    RefreshRadio();
    return;
  }
  app_->radio_services()->FetchRadioBrowser(radio_query_);
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
  ClearList(devices_list_);
  if (!device_browse_id_.empty()) {
    GtkWidget *back = AppendStringRow(GTK_LIST_BOX(devices_list_), "← Devices", nullptr);
    g_object_set_data(G_OBJECT(back), "row-kind", const_cast<char *>("back"));
    GtkWidget *add_all = AppendStringRow(GTK_LIST_BOX(devices_list_), "Add all to playlist", nullptr);
    g_object_set_data(G_OBJECT(add_all), "row-kind", const_cast<char *>("add-all"));
    const SongList songs = app_->device_manager()->Songs(device_browse_id_);
    for (const Song &song : songs) {
      auto *copy = new Song(song);
      GtkWidget *row = AppendStringRow(GTK_LIST_BOX(devices_list_), song.PrettyTitleWithArtist(), copy, [](gpointer p) {
        delete static_cast<Song *>(p);
      });
      g_object_set_data(G_OBJECT(row), "row-kind", const_cast<char *>("song"));
    }
    if (songs.empty()) {
      AppendStringRow(GTK_LIST_BOX(devices_list_), "No songs found on this device", nullptr);
    }
    return;
  }
  for (const ConnectedDevice &device : app_->device_manager()->devices()) {
    GtkWidget *row = AppendStringRow(GTK_LIST_BOX(devices_list_), device.friendly_name + " · " + device.backend,
                                     g_strdup(device.unique_id.c_str()), g_free);
    g_object_set_data(G_OBJECT(row), "row-kind", const_cast<char *>("device"));
  }
  if (app_->device_manager()->devices().empty()) {
    AppendStringRow(GTK_LIST_BOX(devices_list_), "No devices found", nullptr);
  }
}

void MainWindow::RefreshFiles() {
  if (file_view_) {
    file_view_->Reload();
  }
}

void MainWindow::UpdateNowPlaying() {
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
  }
}

void MainWindow::UpdateCover(const std::vector<unsigned char> &data) {
  if (playing_widget_) {
    playing_widget_->SetCover(data);
  }
  if (context_view_) {
    context_view_->AlbumCoverLoaded(data);
  }
}

void MainWindow::UpdatePlaybackButtons() {
  const bool playing = app_->player()->GetState() == GstEngine::State::Playing;
  gtk_button_set_icon_name(GTK_BUTTON(play_button_), playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic");
}

void MainWindow::OpenSettings() { SettingsDialog::Show(GTK_WINDOW(window_), app_); }

void MainWindow::OpenAbout() {
  AdwDialog *about = adw_about_dialog_new();
  adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about), "Strawberry");
  adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about), "strawberry");
  adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about), STRAWBERRY_VERSION_DISPLAY);
  adw_about_dialog_set_developer_name(ADW_ABOUT_DIALOG(about), "Jonas Kvinge and contributors");
  adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about), GTK_LICENSE_GPL_3_0);
  adw_about_dialog_set_comments(ADW_ABOUT_DIALOG(about), "Music player and collection organizer, rewritten with GTK 4 and libadwaita.");
  adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about), "https://www.strawberrymusicplayer.org");
  adw_dialog_present(about, GTK_WIDGET(window_));
}

void MainWindow::AddFiles() {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Open audio files");
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
  gtk_file_dialog_set_title(dialog, "Add collection folder");
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
  gtk_file_dialog_set_title(dialog, "Load playlist");
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
      self->app_->playlist_manager()->AppendSongs(PlaylistParser().Load(path));
      self->RefreshPlaylist();
      g_free(path);
    }
    g_object_unref(file);
  }, this);
}

void MainWindow::SavePlaylistFile() {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Save playlist");
  gtk_file_dialog_save(dialog, GTK_WINDOW(window_), nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *self = static_cast<MainWindow *>(data);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!file) {
      if (error) g_error_free(error);
      return;
    }
    gchar *path = g_file_get_path(file);
    if (path && self->app_->playlist_manager()->active()) {
      PlaylistParser().Save(path, self->app_->playlist_manager()->active()->songs());
      g_free(path);
    }
    g_object_unref(file);
  }, this);
}

void MainWindow::NewPlaylist() {
  app_->playlist_manager()->New("Playlist " + std::to_string(app_->playlist_manager()->playlists().size() + 1));
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::ClearPlaylist() {
  if (app_->playlist_manager()->active()) {
    app_->playlist_manager()->active()->Clear();
    app_->playlist_manager()->SaveActive();
    RefreshPlaylist();
  }
}

void MainWindow::UndoPlaylist() {
  if (Playlist *playlist = app_->playlist_manager()->active()) {
    playlist->Undo();
    app_->playlist_manager()->SaveActive();
    RefreshPlaylist();
  }
}

void MainWindow::RedoPlaylist() {
  if (Playlist *playlist = app_->playlist_manager()->active()) {
    playlist->Redo();
    app_->playlist_manager()->SaveActive();
    RefreshPlaylist();
  }
}

void MainWindow::CycleAnalyzer() {
  const auto types = Analyzer::Types();
  auto it = std::find(types.begin(), types.end(), app_->analyzer()->type());
  const size_t index = it == types.end() ? 0 : (static_cast<size_t>(std::distance(types.begin(), it)) + 1) % types.size();
  app_->analyzer()->set_type(types[index]);
  gtk_widget_set_tooltip_text(analyzer_drawing_, ("Analyzer: " + types[index]).c_str());
  gtk_widget_queue_draw(analyzer_drawing_);
}

void MainWindow::CycleRepeat() {
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist) {
    return;
  }
  playlist_sequence_.CycleRepeatMode();
  playlist->SetRepeatMode(playlist_sequence_.repeat_mode());
  gtk_widget_set_tooltip_text(repeat_button_, PlaylistSequence::RepeatLabel(playlist_sequence_.repeat_mode()));
}

void MainWindow::CycleShuffle() {
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist) {
    return;
  }
  playlist_sequence_.CycleShuffleMode();
  playlist->SetShuffleMode(playlist_sequence_.shuffle_mode());
  gtk_widget_set_tooltip_text(shuffle_button_, PlaylistSequence::ShuffleLabel(playlist_sequence_.shuffle_mode()));
  if (playlist_sequence_.shuffle_mode() == PlaylistSequence::ShuffleMode::All) {
    playlist->Shuffle();
    app_->playlist_manager()->SaveActive();
    RefreshPlaylist();
  }
}

void MainWindow::RunSmartPlaylist(const std::string &kind) {
  SmartPlaylistSearch search;
  std::string name = "Smart playlist";
  if (StrUtils::StartsWith(kind, "saved:")) {
    const std::string saved_name = kind.substr(6);
    for (const auto &preset : SmartPlaylistSearch::LoadSaved()) {
      if (preset.first == saved_name) {
        search = preset.second;
        name = saved_name;
        break;
      }
    }
  } else if (kind == "never") {
    search.terms.push_back({SmartPlaylistField::Playcount, SmartPlaylistOp::Equals, "0"});
    name = "Never played";
  } else if (kind == "rated") {
    search.sort_field = SmartPlaylistField::Rating;
    search.sort_descending = true;
    search.limit = 100;
    name = "Highest rated";
  } else if (kind == "newest") {
    search.sort_field = SmartPlaylistField::Year;
    search.sort_descending = true;
    search.limit = 100;
    name = "Newest";
  } else if (kind == "played") {
    search.sort_field = SmartPlaylistField::Playcount;
    search.sort_descending = true;
    search.limit = 100;
    name = "Most played";
  }
  Playlist *playlist = app_->playlist_manager()->New(name);
  playlist->SetDynamic(true, search);
  SmartPlaylistSearch initial = search;
  if (initial.limit == 0 || initial.limit > 20) {
    initial.limit = 20;
  }
  app_->playlist_manager()->AppendSongs(initial.Search(app_->collection()->Songs()));
  RefreshPlaylistsList();
  RefreshPlaylist();
}

void MainWindow::PlayRadioChannel(const RadioChannel &channel) {
  Song song(channel.source);
  song.set_title(channel.name);
  song.set_url(channel.url);
  song.set_art_automatic(channel.thumbnail_url);
  song.set_genre(channel.tags);
  song.set_valid(true);
  app_->playlist_manager()->AppendSongs({song});
  RefreshPlaylist();
  if (app_->playlist_manager()->active()) {
    app_->player()->PlayAt(app_->playlist_manager()->active()->row_count() - 1);
  }
}

void MainWindow::ShowPlaylistMenu(double, double) {
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Play", "win.playlist-play");
  g_menu_append(menu, "Queue", "win.playlist-queue");
  g_menu_append(menu, "Remove", "win.playlist-remove");
  g_menu_append(menu, "Edit tags…", "win.edittag");
  g_menu_append(menu, "Cover search…", "win.cover-search");
  g_menu_append(menu, "Delete file…", "win.delete-files");
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, playlist_container_ ? playlist_container_->view()->grid() : GTK_WIDGET(window_));
  gtk_popover_popup(GTK_POPOVER(popover));
}

std::vector<int> MainWindow::SelectedPlaylistRows() const {
  if (!selected_playlist_rows_.empty()) {
    return selected_playlist_rows_;
  }
  const int current = app_->playlist_manager()->current_row();
  if (current >= 0) {
    return {current};
  }
  return {};
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
  const auto &bands = self->app_->analyzer()->bands();
  if (bands.empty() || width <= 0) {
    return;
  }
  const std::string type = self->app_->analyzer()->type();
  const double bar_w = static_cast<double>(width) / static_cast<double>(bands.size());
  if (type == "Wave" || type == "Sonic") {
    cairo_set_source_rgb(cr, type == "Sonic" ? 0.95 : 0.23, 0.63, type == "Sonic" ? 0.35 : 0.95);
    cairo_move_to(cr, 0, height / 2.0);
    for (size_t i = 0; i < bands.size(); ++i) {
      const double x = static_cast<double>(i) * bar_w;
      const double y = height / 2.0 - static_cast<double>(bands[i]) * height / 2.0;
      cairo_line_to(cr, x, y);
    }
    cairo_stroke(cr);
    return;
  }
  for (size_t i = 0; i < bands.size(); ++i) {
    const double h = std::max(1.0, static_cast<double>(bands[i]) * height);
    if (type == "Rainbow") {
      cairo_set_source_rgb(cr, static_cast<double>(i) / bands.size(), 0.4, 1.0 - static_cast<double>(i) / bands.size());
    } else if (type == "Turbine") {
      cairo_set_source_rgb(cr, 0.9, 0.45 + bands[i] * 0.4, 0.1);
    } else if (type == "Block") {
      cairo_set_source_rgb(cr, 0.2, 0.8, 0.4);
    } else {
      cairo_set_source_rgb(cr, 0.23, 0.63, 0.95);
    }
    if (type == "Block") {
      const int blocks = std::max(1, static_cast<int>(h / 4));
      for (int b = 0; b < blocks; ++b) {
        cairo_rectangle(cr, static_cast<double>(i) * bar_w, height - (b + 1) * 4, bar_w - 1.0, 3);
        cairo_fill(cr);
      }
    } else {
      cairo_rectangle(cr, static_cast<double>(i) * bar_w, height - h, bar_w - 1.0, h);
      cairo_fill(cr);
    }
  }
}

void MainWindow::DrawMoodbar(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  const auto &mood = self->app_->moodbar()->data();
  if (mood.empty() || width <= 0) {
    return;
  }
  for (int x = 0; x < width; ++x) {
    const size_t i = static_cast<size_t>(x) * mood.size() / static_cast<size_t>(width);
    const double v = mood[i] / 255.0;
    cairo_set_source_rgb(cr, 0.2 + v * 0.6, 0.1 + v * 0.4, 0.7 - v * 0.3);
    cairo_rectangle(cr, x, 0, 1, height);
    cairo_fill(cr);
  }
}

void MainWindow::DrawWaveform(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  const auto &wave = self->app_->waveform()->data();
  if (wave.empty() || width <= 0) {
    return;
  }
  cairo_set_source_rgb(cr, 0.35, 0.72, 0.45);
  cairo_move_to(cr, 0, height / 2.0);
  for (int x = 0; x < width; ++x) {
    const size_t i = static_cast<size_t>(x) * wave.size() / static_cast<size_t>(width);
    const double amp = std::max(0.05, static_cast<double>(wave[i]));
    cairo_line_to(cr, x, height / 2.0 - amp * height / 2.0);
  }
  cairo_stroke(cr);
}
