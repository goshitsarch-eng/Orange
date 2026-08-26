#include "ui/mainwindow.h"

#include "core/settings.h"
#include "playlistparsers/playlistparser.h"
#include "smartplaylists/smartplaylist.h"
#include "ui/dialogs.h"
#include "ui/settingsdialog.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

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

void ClearBox(GtkWidget *box) {
  GtkWidget *child = gtk_widget_get_first_child(box);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
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

GtkWidget *ColLabel(const std::string &text, int width, bool expand, bool heading) {
  GtkWidget *label = gtk_label_new(text.c_str());
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_margin_start(label, 6);
  gtk_widget_set_margin_end(label, 6);
  if (heading) {
    gtk_widget_add_css_class(label, "heading");
  }
  if (expand) {
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_FILL);
  } else {
    gtk_widget_set_size_request(label, width, -1);
  }
  return label;
}

const char *HomeOrMusic() {
  const char *music = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
  if (music && *music) {
    return music;
  }
  return g_get_home_dir();
}

}  // namespace

MainWindow::MainWindow(AdwApplication *gtk_app, Application *app, const CommandlineOptions &options)
    : gtk_app_(gtk_app), app_(app), files_path_(HomeOrMusic() ? HomeOrMusic() : ".") {
  Settings settings;
  settings.BeginGroup("Collection");
  collection_group_ = settings.Value("groupby", "artist-album");
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
  g_menu_append(tools, "Cover from URL…", "win.cover-from-url");
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
                     self->RefreshCollection(gtk_editable_get_text(GTK_EDITABLE(entry)));
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
  gtk_box_append(GTK_BOX(content), playlist_scroll_);
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
  add_action("cover-from-url", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverFromUrl(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
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
               Dialogs::GroupBy(GTK_WINDOW(self->window_), [self](const std::string &group) {
                 self->collection_group_ = group;
                 Settings settings;
                 settings.BeginGroup("Collection");
                 settings.SetValue("groupby", group);
                 settings.Sync();
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
               if (self->app_->playlist_manager()->active()) {
                 self->app_->queue()->Append(self->app_->playlist_manager()->current_song());
                 self->RefreshQueue();
               }
             }));
  add_action("playlist-remove", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               if (Playlist *playlist = self->app_->playlist_manager()->active()) {
                 playlist->RemoveRows({self->app_->playlist_manager()->current_row()});
                 self->app_->playlist_manager()->SaveActive();
                 self->RefreshPlaylist();
               }
             }));
}

void MainWindow::BuildSidebar() {
  sidebar_stack_ = ADW_VIEW_STACK(adw_view_stack_new());
  gtk_widget_set_vexpand(GTK_WIDGET(sidebar_stack_), TRUE);

  BuildContext();
  adw_view_stack_add_titled_with_icon(sidebar_stack_, GTK_WIDGET(g_object_get_data(G_OBJECT(context_cover_), "context-scroll")), "context",
                                      "Context", "audio-x-generic-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&collection_list_), "collection", "Collection",
                                      "media-optical-cd-audio-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&playlists_list_), "playlists", "Playlists", "view-list-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&smart_list_), "smart", "Smart playlists", "view-refresh-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&files_list_), "files", "Files", "folder-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&radio_list_), "radio", "Internet radio", "network-wireless-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&streaming_list_), "streaming", "Streaming", "emblem-shared-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&devices_list_), "devices", "Devices", "drive-harddisk-usb-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&queue_list_), "queue", "Queue", "view-list-ordered-symbolic");

  g_signal_connect(collection_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song) {
                       self->app_->playlist_manager()->AppendSongs({*song});
                       self->RefreshPlaylist();
                     }
                   }),
                   this);
  g_signal_connect(playlists_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-text"));
                     if (name) {
                       self->app_->playlist_manager()->SetCurrentPlaylist(name);
                       self->RefreshPlaylist();
                     }
                   }),
                   this);
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
  g_signal_connect(files_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (!path) {
                       return;
                     }
                     if (FileUtils::IsDirectory(path)) {
                       self->files_path_ = path;
                       self->RefreshFiles();
                       return;
                     }
                     if (PlaylistParser::IsPlaylist(path)) {
                       self->app_->playlist_manager()->AppendSongs(PlaylistParser().Load(path));
                     } else {
                       self->app_->playlist_manager()->InsertUrls({FileUtils::UriFromPath(path)});
                     }
                     self->RefreshPlaylist();
                   }),
                   this);
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
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  context_cover_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(context_cover_), 220);
  gtk_widget_set_halign(context_cover_, GTK_ALIGN_CENTER);
  context_title_ = gtk_label_new("Not playing");
  gtk_widget_add_css_class(context_title_, "title-2");
  gtk_label_set_wrap(GTK_LABEL(context_title_), TRUE);
  context_artist_ = gtk_label_new("");
  gtk_widget_add_css_class(context_artist_, "heading");
  context_album_ = gtk_label_new("");
  gtk_widget_add_css_class(context_album_, "dim-label");
  context_meta_ = gtk_label_new("");
  gtk_widget_add_css_class(context_meta_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(context_meta_), TRUE);
  lyrics_view_ = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(lyrics_view_), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(lyrics_view_), FALSE);
  gtk_widget_set_vexpand(lyrics_view_, TRUE);
  gtk_box_append(GTK_BOX(box), context_cover_);
  gtk_box_append(GTK_BOX(box), context_title_);
  gtk_box_append(GTK_BOX(box), context_artist_);
  gtk_box_append(GTK_BOX(box), context_album_);
  gtk_box_append(GTK_BOX(box), context_meta_);
  gtk_box_append(GTK_BOX(box), lyrics_view_);
  GtkWidget *save_lyrics = gtk_button_new_with_label("Save lyrics to tag");
  g_signal_connect(save_lyrics, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     Song song = self->app_->player()->current_song();
                     GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->lyrics_view_));
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buffer, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                     song.set_lyrics(text ? text : "");
                     g_free(text);
                     if (self->app_->tagreader()->WriteFile(song)) {
                       gtk_button_set_label(btn, "Saved");
                       if (song.id() > 0) {
                         self->app_->collection()->backend()->AddOrUpdateSong(song);
                       }
                     }
                   })),
                   this);
  gtk_box_append(GTK_BOX(box), save_lyrics);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), box);
  g_object_set_data(G_OBJECT(context_cover_), "context-scroll", scroll);
}

void MainWindow::BuildPlaylist() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(toolbar, 8);
  gtk_widget_set_margin_end(toolbar, 8);
  gtk_widget_set_margin_top(toolbar, 8);
  gtk_widget_set_margin_bottom(toolbar, 4);
  auto add_tool = [&](const char *icon, const char *tooltip, GCallback callback) {
    GtkWidget *button = gtk_button_new_from_icon_name(icon);
    gtk_widget_set_tooltip_text(button, tooltip);
    g_signal_connect(button, "clicked", callback, this);
    gtk_box_append(GTK_BOX(toolbar), button);
  };
  add_tool("document-new-symbolic", "New playlist", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->NewPlaylist(); }));
  add_tool("document-open-symbolic", "Load playlist", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->LoadPlaylistFile(); }));
  add_tool("document-save-symbolic", "Save playlist", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->SavePlaylistFile(); }));
  add_tool("edit-clear-all-symbolic", "Clear playlist", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->ClearPlaylist(); }));
  add_tool("edit-undo-symbolic", "Undo", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->UndoPlaylist(); }));
  add_tool("edit-redo-symbolic", "Redo", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->RedoPlaylist(); }));
  repeat_button_ = gtk_button_new_from_icon_name("media-playlist-repeat-symbolic");
  gtk_widget_set_tooltip_text(repeat_button_, "Cycle repeat");
  g_signal_connect(repeat_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->CycleRepeat(); }), this);
  shuffle_button_ = gtk_button_new_from_icon_name("media-playlist-shuffle-symbolic");
  gtk_widget_set_tooltip_text(shuffle_button_, "Shuffle playlist");
  g_signal_connect(shuffle_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->CycleShuffle(); }), this);
  gtk_box_append(GTK_BOX(toolbar), repeat_button_);
  gtk_box_append(GTK_BOX(toolbar), shuffle_button_);
  GtkWidget *filter = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(filter), "Filter playlist");
  gtk_widget_set_hexpand(filter, TRUE);
  g_signal_connect(filter, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     self->playlist_filter_ = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->RefreshPlaylist();
                   }),
                   this);
  gtk_box_append(GTK_BOX(toolbar), filter);
  playlist_summary_ = gtk_label_new("");
  gtk_widget_add_css_class(playlist_summary_, "dim-label");
  gtk_box_append(GTK_BOX(toolbar), playlist_summary_);
  gtk_box_append(GTK_BOX(box), toolbar);
  playlist_tabs_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(playlist_tabs_, 8);
  gtk_widget_set_margin_end(playlist_tabs_, 8);
  gtk_box_append(GTK_BOX(box), playlist_tabs_);

  playlist_scroll_ = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(playlist_scroll_, TRUE);
  gtk_widget_set_vexpand(playlist_scroll_, TRUE);
  playlist_grid_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(playlist_scroll_), playlist_grid_);
  gtk_box_append(GTK_BOX(box), playlist_scroll_);
  playlist_scroll_ = box;

  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(playlist_grid_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble y, gpointer data) {
                     static_cast<MainWindow *>(data)->ShowPlaylistMenu(x, y);
                   }),
                   this);
}

void MainWindow::BuildPlayerBar() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  GtkWidget *now = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  cover_image_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(cover_image_), 48);
  GtkWidget *labels = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  title_label_ = gtk_label_new("Not playing");
  gtk_widget_add_css_class(title_label_, "heading");
  gtk_widget_set_halign(title_label_, GTK_ALIGN_START);
  artist_label_ = gtk_label_new("");
  gtk_widget_add_css_class(artist_label_, "dim-label");
  gtk_widget_set_halign(artist_label_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(labels), title_label_);
  gtk_box_append(GTK_BOX(labels), artist_label_);
  gtk_box_append(GTK_BOX(now), cover_image_);
  gtk_box_append(GTK_BOX(now), labels);
  gtk_box_append(GTK_BOX(box), now);

  waveform_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(waveform_drawing_, -1, 28);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(waveform_drawing_), DrawWaveform, this, nullptr);
  gtk_box_append(GTK_BOX(box), waveform_drawing_);
  moodbar_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(moodbar_drawing_, -1, 16);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(moodbar_drawing_), DrawMoodbar, this, nullptr);
  gtk_box_append(GTK_BOX(box), moodbar_drawing_);

  GtkWidget *seek_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  position_label_ = gtk_label_new("0:00");
  seek_scale_ = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1000, 1);
  gtk_widget_set_hexpand(seek_scale_, TRUE);
  duration_label_ = gtk_label_new("0:00");
  gtk_box_append(GTK_BOX(seek_row), position_label_);
  gtk_box_append(GTK_BOX(seek_row), seek_scale_);
  gtk_box_append(GTK_BOX(seek_row), duration_label_);
  gtk_box_append(GTK_BOX(box), seek_row);

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
  volume_scale_ = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_range_set_value(GTK_RANGE(volume_scale_), app_->player()->GetVolume());
  gtk_widget_set_size_request(volume_scale_, 120, -1);
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
  gtk_box_append(GTK_BOX(controls), volume_scale_);
  gtk_box_append(GTK_BOX(box), controls);

  status_label_ = gtk_label_new("Ready");
  gtk_widget_add_css_class(status_label_, "dim-label");
  gtk_box_append(GTK_BOX(box), status_label_);

  g_signal_connect(play_button_, "clicked", G_CALLBACK(OnPlayPause), this);
  g_signal_connect(stop, "clicked", G_CALLBACK(OnStop), this);
  g_signal_connect(next, "clicked", G_CALLBACK(OnNext), this);
  g_signal_connect(prev, "clicked", G_CALLBACK(OnPrevious), this);
  g_signal_connect(love, "clicked", G_CALLBACK(OnLove), this);
  g_signal_connect(volume_scale_, "value-changed", G_CALLBACK(OnVolume), this);
  g_signal_connect(seek_scale_, "value-changed", G_CALLBACK(OnSeek), this);
  g_object_set_data(G_OBJECT(play_button_), "player-box", box);
}

void MainWindow::ConnectSignals() {
  app_->player()->SongChanged.Connect([this](const Song &) { UpdateNowPlaying(); });
  app_->player()->StateChanged.Connect([this](GstEngine::State) { UpdatePlaybackButtons(); });
  app_->player()->VolumeChanged.Connect([this](unsigned volume) { gtk_range_set_value(GTK_RANGE(volume_scale_), volume); });
  app_->playlist_manager()->PlaylistsLoaded.Connect([this]() {
    RefreshPlaylistsList();
    RefreshPlaylist();
  });
  app_->collection()->ScanFinished.Connect([this]() { RefreshCollection(); });
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
    gtk_label_set_text(GTK_LABEL(self->position_label_), Utilities::PrettyTimeNanosec(pos).c_str());
    gtk_label_set_text(GTK_LABEL(self->duration_label_), Utilities::PrettyTimeNanosec(len).c_str());
    if (len > 0) {
      g_signal_handlers_block_by_func(self->seek_scale_, reinterpret_cast<gpointer>(OnSeek), self);
      gtk_range_set_value(GTK_RANGE(self->seek_scale_), 1000.0 * static_cast<double>(pos) / static_cast<double>(len));
      g_signal_handlers_unblock_by_func(self->seek_scale_, reinterpret_cast<gpointer>(OnSeek), self);
    }
    return G_SOURCE_CONTINUE;
  }, this);
}

void MainWindow::RefreshCollection(const std::string &filter) {
  ClearList(collection_list_);
  const SongList songs = app_->collection()->Songs(filter);
  std::string last_header;
  GtkWidget *current_box = nullptr;
  for (const Song &song : songs) {
    const std::string header = CollectionHeader(song);
    if (header != last_header) {
      GtkWidget *expander = gtk_expander_new(header.c_str());
      gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
      current_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_expander_set_child(GTK_EXPANDER(expander), current_box);
      GtkWidget *header_row = gtk_list_box_row_new();
      gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(header_row), FALSE);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(header_row), expander);
      gtk_list_box_append(GTK_LIST_BOX(collection_list_), header_row);
      last_header = header;
    }
    auto *copy = new Song(song);
    const std::string text = (song.track() > 0 ? std::to_string(song.track()) + ". " : "") + song.PrettyTitle();
    if (current_box) {
      GtkWidget *button = gtk_button_new_with_label(text.c_str());
      gtk_widget_add_css_class(button, "flat");
      gtk_widget_set_halign(button, GTK_ALIGN_START);
      g_object_set_data_full(G_OBJECT(button), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
      g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                         auto *self = static_cast<MainWindow *>(data);
                         if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(btn), "song"))) {
                           self->app_->playlist_manager()->AppendSongs({*song});
                           self->RefreshPlaylist();
                         }
                       }),
                       this);
      gtk_box_append(GTK_BOX(current_box), button);
    } else {
      AppendStringRow(GTK_LIST_BOX(collection_list_), text, copy, [](gpointer p) { delete static_cast<Song *>(p); });
    }
  }
  gtk_label_set_text(GTK_LABEL(status_label_), (std::to_string(songs.size()) + " songs").c_str());
}

std::string MainWindow::ColumnTitle(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Track:
      return "Track";
    case PlaylistColumn::Title:
      return "Title";
    case PlaylistColumn::Artist:
      return "Artist";
    case PlaylistColumn::Album:
      return "Album";
    case PlaylistColumn::AlbumArtist:
      return "Album artist";
    case PlaylistColumn::Performer:
      return "Performer";
    case PlaylistColumn::Composer:
      return "Composer";
    case PlaylistColumn::Year:
      return "Year";
    case PlaylistColumn::OriginalYear:
      return "Original year";
    case PlaylistColumn::Disc:
      return "Disc";
    case PlaylistColumn::Length:
      return "Length";
    case PlaylistColumn::Genre:
      return "Genre";
    case PlaylistColumn::Samplerate:
      return "Sample rate";
    case PlaylistColumn::Bitdepth:
      return "Bit depth";
    case PlaylistColumn::Bitrate:
      return "Bitrate";
    case PlaylistColumn::URL:
      return "URL";
    case PlaylistColumn::Filename:
      return "Filename";
    case PlaylistColumn::Filesize:
      return "Filesize";
    case PlaylistColumn::Filetype:
      return "Filetype";
    case PlaylistColumn::DateCreated:
      return "Date created";
    case PlaylistColumn::DateModified:
      return "Date modified";
    case PlaylistColumn::PlayCount:
      return "Plays";
    case PlaylistColumn::SkipCount:
      return "Skips";
    case PlaylistColumn::LastPlayed:
      return "Last played";
    case PlaylistColumn::Comment:
      return "Comment";
    case PlaylistColumn::Grouping:
      return "Grouping";
    case PlaylistColumn::Source:
      return "Source";
    case PlaylistColumn::Moodbar:
      return "Moodbar";
    case PlaylistColumn::Rating:
      return "Rating";
    case PlaylistColumn::HasCUE:
      return "CUE";
    case PlaylistColumn::EBUR128I:
      return "EBU R128 I";
    case PlaylistColumn::EBUR128LRA:
      return "EBU R128 LRA";
    case PlaylistColumn::BPM:
      return "BPM";
    case PlaylistColumn::Mood:
      return "Mood";
    case PlaylistColumn::InitialKey:
      return "Initial key";
    case PlaylistColumn::Count:
      break;
  }
  return {};
}

int MainWindow::ColumnWidth(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Title:
    case PlaylistColumn::URL:
    case PlaylistColumn::Filename:
    case PlaylistColumn::Comment:
      return 200;
    case PlaylistColumn::Artist:
    case PlaylistColumn::Album:
    case PlaylistColumn::AlbumArtist:
    case PlaylistColumn::Performer:
    case PlaylistColumn::Composer:
      return 150;
    default:
      return 80;
  }
}

bool MainWindow::ColumnVisible(PlaylistColumn column) const {
  Settings settings;
  settings.BeginGroup("Playlist");
  const std::string enabled =
      settings.Value("columns", "Track,Title,Artist,Album,Album artist,Length,Year,Genre,Bitrate,Sample rate,Plays,Rating,Filename");
  const std::string title = ColumnTitle(column);
  for (const std::string &part : StrUtils::Split(enabled, ',')) {
    if (part == title) {
      return true;
    }
  }
  return false;
}

std::string MainWindow::ColumnText(const Song &song, PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Track:
      return song.track() > 0 ? std::to_string(song.track()) : "";
    case PlaylistColumn::Title:
      return song.PrettyTitle();
    case PlaylistColumn::Artist:
      return song.artist();
    case PlaylistColumn::Album:
      return song.album();
    case PlaylistColumn::AlbumArtist:
      return song.EffectiveAlbumartist();
    case PlaylistColumn::Performer:
      return song.performer();
    case PlaylistColumn::Composer:
      return song.composer();
    case PlaylistColumn::Year:
      return song.year() > 0 ? std::to_string(song.year()) : "";
    case PlaylistColumn::OriginalYear:
      return song.originalyear() > 0 ? std::to_string(song.originalyear()) : "";
    case PlaylistColumn::Disc:
      return song.disc() > 0 ? std::to_string(song.disc()) : "";
    case PlaylistColumn::Length:
      return Utilities::PrettyTimeNanosec(song.length_nanosec());
    case PlaylistColumn::Genre:
      return song.genre();
    case PlaylistColumn::Bitrate:
      return song.bitrate() > 0 ? std::to_string(song.bitrate()) : "";
    case PlaylistColumn::Samplerate:
      return song.samplerate() > 0 ? std::to_string(song.samplerate()) : "";
    case PlaylistColumn::Bitdepth:
      return song.bitdepth() > 0 ? std::to_string(song.bitdepth()) : "";
    case PlaylistColumn::URL:
      return song.url();
    case PlaylistColumn::Filename:
      return song.basefilename().empty() ? FileUtils::BaseName(FileUtils::PathFromUri(song.url())) : song.basefilename();
    case PlaylistColumn::Filesize:
      return song.filesize() > 0 ? std::to_string(song.filesize()) : "";
    case PlaylistColumn::Filetype:
      return Song::FiletypeToString(song.filetype());
    case PlaylistColumn::DateCreated:
      return song.ctime() > 0 ? std::to_string(song.ctime()) : "";
    case PlaylistColumn::DateModified:
      return song.mtime() > 0 ? std::to_string(song.mtime()) : "";
    case PlaylistColumn::PlayCount:
      return std::to_string(song.playcount());
    case PlaylistColumn::SkipCount:
      return std::to_string(song.skipcount());
    case PlaylistColumn::LastPlayed:
      return song.lastplayed() > 0 ? std::to_string(song.lastplayed()) : "";
    case PlaylistColumn::Comment:
      return song.comment();
    case PlaylistColumn::Grouping:
      return song.grouping();
    case PlaylistColumn::Source:
      return Song::SourceToString(song.source());
    case PlaylistColumn::Moodbar:
      return song.mood().empty() ? "" : "●";
    case PlaylistColumn::Rating:
      return song.rating() >= 0 ? std::to_string(song.rating()) : "";
    case PlaylistColumn::HasCUE:
      return song.cue_path().empty() ? "" : "CUE";
    case PlaylistColumn::EBUR128I:
      return song.ebur128_integrated_loudness_lufs() ? std::to_string(*song.ebur128_integrated_loudness_lufs()) : "";
    case PlaylistColumn::EBUR128LRA:
      return song.ebur128_loudness_range_lu() ? std::to_string(*song.ebur128_loudness_range_lu()) : "";
    case PlaylistColumn::BPM:
      return song.bpm() > 0 ? std::to_string(song.bpm()) : "";
    case PlaylistColumn::Mood:
      return song.mood();
    case PlaylistColumn::InitialKey:
      return song.initial_key();
    case PlaylistColumn::Count:
      break;
  }
  return {};
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
    const std::string left = ColumnText(a, sort_column_);
    const std::string right = ColumnText(b, sort_column_);
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

void MainWindow::RefreshPlaylist() {
  ClearBox(playlist_grid_);
  Playlist *playlist = app_->playlist_manager()->active();
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(header, "toolbar");
  for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
    const auto column = static_cast<PlaylistColumn>(i);
    if (!ColumnVisible(column)) {
      continue;
    }
    GtkWidget *button = gtk_button_new_with_label(ColumnTitle(column).c_str());
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_hexpand(button, column == PlaylistColumn::Title);
    gtk_widget_set_size_request(button, ColumnWidth(column), -1);
    g_object_set_data(G_OBJECT(button), "column", GINT_TO_POINTER(i));
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       static_cast<MainWindow *>(data)->SortPlaylistBy(static_cast<PlaylistColumn>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "column"))));
                     }),
                     this);
    gtk_box_append(GTK_BOX(header), button);
  }
  gtk_box_append(GTK_BOX(playlist_grid_), header);
  if (!playlist) {
    return;
  }
  const int current = playlist->current_row();
  int visible = 0;
  for (int index = 0; index < playlist->row_count(); ++index) {
    const Song &song = playlist->songs()[static_cast<size_t>(index)];
    if (!playlist_filter_.empty() && !StrUtils::ContainsInsensitive(song.PrettyTitleWithArtist() + " " + song.album(), playlist_filter_)) {
      continue;
    }
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(row, "activatable");
    if (index == current) {
      gtk_widget_add_css_class(row, "accent");
    }
    for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
      const auto column = static_cast<PlaylistColumn>(i);
      if (!ColumnVisible(column)) {
        continue;
      }
      gtk_box_append(GTK_BOX(row), ColLabel(ColumnText(song, column), ColumnWidth(column), column == PlaylistColumn::Title, false));
    }
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index));
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *gesture, gint n_press, gdouble, gdouble, gpointer data) {
                       if (n_press < 2) {
                         return;
                       }
                       auto *self = static_cast<MainWindow *>(data);
                       GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
                       self->app_->player()->PlayAt(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "row-index")));
                     }),
                     this);
    gtk_box_append(GTK_BOX(playlist_grid_), row);
    ++visible;
  }
  gtk_label_set_text(GTK_LABEL(playlist_summary_),
                     (playlist->name() + " · " + std::to_string(visible) + " tracks · " + Utilities::PrettyTimeNanosec(playlist->total_length_nanosec())).c_str());
}

void MainWindow::RefreshPlaylistsList() {
  ClearList(playlists_list_);
  for (const auto &playlist : app_->playlist_manager()->playlists()) {
    AppendStringRow(GTK_LIST_BOX(playlists_list_), playlist->name(), nullptr);
  }
  RefreshPlaylistTabs();
}

void MainWindow::RefreshPlaylistTabs() {
  if (!playlist_tabs_) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(playlist_tabs_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
  for (const auto &playlist : app_->playlist_manager()->playlists()) {
    GtkWidget *button = gtk_toggle_button_new_with_label(playlist->name().c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), playlist.get() == app_->playlist_manager()->active());
    g_object_set_data_full(G_OBJECT(button), "playlist-name", g_strdup(playlist->name().c_str()), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<MainWindow *>(data);
                       const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "playlist-name"));
                       if (name) {
                         self->app_->playlist_manager()->SetCurrentPlaylist(name);
                         self->RefreshPlaylist();
                         self->RefreshPlaylistTabs();
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(playlist_tabs_), button);
  }
}

std::string MainWindow::CollectionHeader(const Song &song) const {
  if (collection_group_ == "album") {
    return song.album().empty() ? "Unknown album" : song.album();
  }
  if (collection_group_ == "genre") {
    return song.genre().empty() ? "Unknown genre" : song.genre();
  }
  if (collection_group_ == "year") {
    return song.year() > 0 ? std::to_string(song.year()) : "Unknown year";
  }
  if (collection_group_ == "artist") {
    return song.EffectiveAlbumartist().empty() ? "Unknown artist" : song.EffectiveAlbumartist();
  }
  return song.EffectiveAlbumartist() + " – " + song.album();
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
  if (app_->radio_services()->channels().empty()) {
    app_->radio_services()->FetchSomaFM();
    app_->radio_services()->FetchRadioParadise();
  }
  for (const RadioChannel &channel : app_->radio_services()->channels()) {
    auto *copy = new RadioChannel(channel);
    AppendStringRow(GTK_LIST_BOX(radio_list_), channel.name.empty() ? channel.url : channel.name, copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
  }
  if (app_->radio_services()->channels().empty()) {
    AppendStringRow(GTK_LIST_BOX(radio_list_), "Radio Paradise", nullptr);
    AppendStringRow(GTK_LIST_BOX(radio_list_), "SomaFM", nullptr);
    AppendStringRow(GTK_LIST_BOX(radio_list_), "Radio Browser", nullptr);
  }
}

void MainWindow::RefreshStreaming() {
  ClearList(streaming_list_);
  for (StreamingService *service : app_->streaming_services()->All()) {
    AppendStringRow(GTK_LIST_BOX(streaming_list_), service->name() + (service->logged_in() ? " (signed in)" : ""), nullptr);
  }
  if (app_->streaming_services()->All().empty()) {
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Subsonic — enable in Preferences", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Tidal — enable in Preferences", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Spotify — enable in Preferences", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Qobuz — enable in Preferences", nullptr);
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
  ClearList(files_list_);
  AppendStringRow(GTK_LIST_BOX(files_list_), "..", g_strdup(FileUtils::DirName(files_path_).c_str()), g_free);
  std::vector<std::string> entries = FileUtils::ListDirectory(files_path_);
  std::sort(entries.begin(), entries.end());
  for (const std::string &path : entries) {
    const std::string name = FileUtils::BaseName(path);
    if (name.empty() || name[0] == '.') {
      continue;
    }
    const bool dir = FileUtils::IsDirectory(path);
    if (!dir && !Song::IsAudioFile(path) && !PlaylistParser::IsPlaylist(path)) {
      continue;
    }
    AppendStringRow(GTK_LIST_BOX(files_list_), (dir ? "📁 " : "🎵 ") + name, g_strdup(path.c_str()), g_free);
  }
}

void MainWindow::UpdateNowPlaying() {
  const Song song = app_->player()->current_song();
  gtk_label_set_text(GTK_LABEL(title_label_), song.PrettyTitle().c_str());
  gtk_label_set_text(GTK_LABEL(artist_label_), song.EffectiveAlbumartist().c_str());
  gtk_label_set_text(GTK_LABEL(context_title_), song.PrettyTitle().empty() ? "Not playing" : song.PrettyTitle().c_str());
  gtk_label_set_text(GTK_LABEL(context_artist_), song.artist().c_str());
  gtk_label_set_text(GTK_LABEL(context_album_), song.album().c_str());
  const std::string meta = Song::SourceToString(song.source()) + " · " +
                           (song.bitrate() > 0 ? std::to_string(song.bitrate()) + " kbps · " : "") +
                           (song.samplerate() > 0 ? std::to_string(song.samplerate()) + " Hz · " : "") +
                           (song.bitdepth() > 0 ? std::to_string(song.bitdepth()) + "-bit · " : "") +
                           Utilities::PrettyTimeNanosec(song.length_nanosec());
  gtk_label_set_text(GTK_LABEL(context_meta_), meta.c_str());
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
  app_->lyrics_providers()->Fetch(song, [this](const std::string &lyrics, const std::string &) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(lyrics_view_));
    gtk_text_buffer_set_text(buffer, lyrics.empty() ? "No lyrics" : lyrics.c_str(), -1);
  });
}

void MainWindow::UpdateCover(const std::vector<unsigned char> &data) {
  SetImageFromBytes(cover_image_, data, 48);
  SetImageFromBytes(context_cover_, data, 220);
}

void MainWindow::SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data, int pixel_size) {
  if (data.empty()) {
    return;
  }
  GBytes *bytes = g_bytes_new(data.data(), data.size());
  GError *error = nullptr;
  GdkTexture *texture = gdk_texture_new_from_bytes(bytes, &error);
  g_bytes_unref(bytes);
  if (!texture) {
    if (error) {
      g_error_free(error);
    }
    return;
  }
  gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
  gtk_image_set_pixel_size(GTK_IMAGE(image), pixel_size);
  g_object_unref(texture);
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
  switch (playlist->sequence_mode()) {
    case Playlist::SequenceMode::Sequential:
      playlist->SetSequenceMode(Playlist::SequenceMode::RepeatAll);
      break;
    case Playlist::SequenceMode::RepeatAll:
      playlist->SetSequenceMode(Playlist::SequenceMode::RepeatTrack);
      break;
    default:
      playlist->SetSequenceMode(Playlist::SequenceMode::Sequential);
      break;
  }
  gtk_widget_set_tooltip_text(repeat_button_, playlist->sequence_mode() == Playlist::SequenceMode::RepeatTrack ? "Repeat track" :
                                              playlist->sequence_mode() == Playlist::SequenceMode::RepeatAll   ? "Repeat playlist"
                                                                                                               : "Repeat off");
}

void MainWindow::CycleShuffle() {
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist) {
    return;
  }
  playlist->Shuffle();
  app_->playlist_manager()->SaveActive();
  RefreshPlaylist();
}

void MainWindow::RunSmartPlaylist(const std::string &kind) {
  SmartPlaylistSearch search;
  std::string name = "Smart playlist";
  if (kind == "never") {
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
  g_menu_append(menu, "Delete file…", "win.delete-files");
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, playlist_grid_);
  gtk_popover_popup(GTK_POPOVER(popover));
}

std::vector<int> MainWindow::SelectedPlaylistRows() const { return {}; }

void MainWindow::OnPlayPause(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->PlayPause(); }
void MainWindow::OnStop(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Stop(); }
void MainWindow::OnNext(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Next(); }
void MainWindow::OnPrevious(GtkButton *, gpointer data) { static_cast<MainWindow *>(data)->app_->player()->Previous(); }
void MainWindow::OnLove(GtkButton *, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  self->app_->scrobbler()->Love(self->app_->player()->current_song());
}
void MainWindow::OnVolume(GtkRange *range, gpointer data) {
  static_cast<MainWindow *>(data)->app_->player()->SetVolume(static_cast<unsigned>(gtk_range_get_value(range)));
}
void MainWindow::OnSeek(GtkRange *range, gpointer data) {
  auto *self = static_cast<MainWindow *>(data);
  const int64_t len = self->app_->player()->engine()->length_nanosec();
  if (len > 0) {
    self->app_->player()->SeekTo(static_cast<int64_t>(gtk_range_get_value(range) / 1000.0 * (len / 1000000000.0)));
  }
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
