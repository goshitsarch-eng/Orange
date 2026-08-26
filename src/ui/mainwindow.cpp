#include "ui/mainwindow.h"

#include "ui/dialogs.h"
#include "ui/settingsdialog.h"
#include "utilities/timeutils.h"
#include "version.h"

#include <algorithm>

namespace {

void AppendStringRow(GtkListBox *list, const std::string &text, gpointer user_data) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *label = gtk_label_new(text.c_str());
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_margin_start(label, 12);
  gtk_widget_set_margin_end(label, 12);
  gtk_widget_set_margin_top(label, 8);
  gtk_widget_set_margin_bottom(label, 8);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
  g_object_set_data_full(G_OBJECT(row), "row-text", g_strdup(text.c_str()), g_free);
  if (user_data) {
    g_object_set_data(G_OBJECT(row), "row-data", user_data);
  }
  gtk_list_box_append(list, row);
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
  BuildUi();
  ConnectSignals();
  RefreshCollection();
  RefreshPlaylistsList();
  RefreshPlaylist();
  RefreshRadio();
  RefreshDevices();
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
  Present();
}

void MainWindow::BuildUi() {
  window_ = ADW_APPLICATION_WINDOW(adw_application_window_new(GTK_APPLICATION(gtk_app_)));
  gtk_window_set_title(GTK_WINDOW(window_), "Strawberry");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1280, 800);

  GtkWidget *header = adw_header_bar_new();
  GtkWidget *menu_button = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
  GMenu *menu = g_menu_new();
  GMenu *music = g_menu_new();
  g_menu_append(music, "Open files…", "win.open-files");
  g_menu_append(music, "Add collection folder…", "win.add-folder");
  g_menu_append(music, "Add stream…", "win.add-stream");
  g_menu_append(menu, nullptr, nullptr);
  g_menu_append_section(menu, "Music", G_MENU_MODEL(music));
  GMenu *tools = g_menu_new();
  g_menu_append(tools, "Cover manager", "win.covers");
  g_menu_append(tools, "Equalizer", "win.equalizer");
  g_menu_append(tools, "Transcode…", "win.transcode");
  g_menu_append(tools, "Organize files…", "win.organize");
  g_menu_append(tools, "Fetch tags…", "win.tagfetch");
  g_menu_append(tools, "Edit tags…", "win.edittag");
  g_menu_append(menu, nullptr, nullptr);
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
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search), "Filter collection");
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
  adw_overlay_split_view_set_sidebar_width_fraction(ADW_OVERLAY_SPLIT_VIEW(split), 0.32);
  BuildSidebar();
  adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(split), gtk_widget_get_parent(GTK_WIDGET(sidebar_stack_)) ? GTK_WIDGET(sidebar_stack_) : GTK_WIDGET(sidebar_stack_));

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
  gtk_box_append(GTK_BOX(content), GTK_WIDGET(g_object_get_data(G_OBJECT(playlist_list_), "scroll")));
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
  add_action("add-stream", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
               auto *self = static_cast<MainWindow *>(data);
               Dialogs::AddStream(GTK_WINDOW(self->window_), [self](const std::string &name, const std::string &url) {
                 self->app_->radio_services()->AddCustomStream(name, url);
                 self->RefreshRadio();
               });
             }));
  add_action("covers", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::CoverManager(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("equalizer", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Equalizer(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_->equalizer()); }));
  add_action("transcode", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Transcode(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("organize", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Organize(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("tagfetch", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::TagFetcher(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("edittag", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::EditTag(GTK_WINDOW(static_cast<MainWindow *>(data)->window_), static_cast<MainWindow *>(data)->app_); }));
  add_action("shortcuts", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) { Dialogs::Shortcuts(GTK_WINDOW(static_cast<MainWindow *>(data)->window_)); }));
}

void MainWindow::BuildSidebar() {
  sidebar_stack_ = ADW_VIEW_STACK(adw_view_stack_new());
  gtk_widget_set_vexpand(GTK_WIDGET(sidebar_stack_), TRUE);

  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&collection_list_), "collection", "Collection", "media-optical-cd-audio-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&playlists_list_), "playlists", "Playlists", "view-list-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&smart_list_), "smart", "Smart playlists", "view-refresh-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&files_list_), "files", "Files", "folder-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&radio_list_), "radio", "Internet radio", "network-wireless-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&streaming_list_), "streaming", "Streaming", "emblem-shared-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&devices_list_), "devices", "Devices", "drive-harddisk-usb-symbolic");
  adw_view_stack_add_titled_with_icon(sidebar_stack_, MakeScrolledList(&queue_list_), "queue", "Queue", "view-list-ordered-symbolic");

  GtkWidget *lyrics_scroll = gtk_scrolled_window_new();
  lyrics_view_ = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(lyrics_view_), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(lyrics_view_), FALSE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(lyrics_scroll), lyrics_view_);
  adw_view_stack_add_titled_with_icon(sidebar_stack_, lyrics_scroll, "lyrics", "Lyrics", "text-x-generic-symbolic");

  g_signal_connect(collection_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song) {
                       self->app_->playlist_manager()->AppendSongs({*song});
                       self->RefreshPlaylist();
                     }
                   }),
                   this);
}

void MainWindow::BuildPlaylist() {
  GtkWidget *scroll = MakeScrolledList(&playlist_list_);
  gtk_widget_set_vexpand(scroll, TRUE);
  g_object_set_data(G_OBJECT(playlist_list_), "scroll", scroll);
  g_signal_connect(playlist_list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<MainWindow *>(data);
                     const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-index"));
                     self->app_->player()->PlayAt(index);
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
  gtk_box_append(GTK_BOX(box), waveform_drawing_);
  moodbar_drawing_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(moodbar_drawing_, -1, 16);
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
  volume_scale_ = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_range_set_value(GTK_RANGE(volume_scale_), app_->player()->GetVolume());
  gtk_widget_set_size_request(volume_scale_, 120, -1);
  gtk_box_append(GTK_BOX(controls), prev);
  gtk_box_append(GTK_BOX(controls), play_button_);
  gtk_box_append(GTK_BOX(controls), stop);
  gtk_box_append(GTK_BOX(controls), next);
  gtk_box_append(GTK_BOX(controls), love);
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

  // Keep a reference so BuildUi can attach this box.
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
  position_timeout_ = g_timeout_add(500, [](gpointer data) -> gboolean {
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
  for (const Song &song : songs) {
    const std::string header = song.EffectiveAlbumartist() + " – " + song.album();
    if (header != last_header) {
      GtkWidget *header_row = gtk_list_box_row_new();
      gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(header_row), FALSE);
      GtkWidget *header_label = gtk_label_new(header.c_str());
      gtk_widget_add_css_class(header_label, "heading");
      gtk_widget_set_halign(header_label, GTK_ALIGN_START);
      gtk_widget_set_margin_start(header_label, 12);
      gtk_widget_set_margin_top(header_label, 10);
      gtk_widget_set_margin_bottom(header_label, 4);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(header_row), header_label);
      gtk_list_box_append(GTK_LIST_BOX(collection_list_), header_row);
      last_header = header;
    }
    auto *copy = new Song(song);
    GtkWidget *row = gtk_list_box_row_new();
    const std::string text = (song.track() > 0 ? std::to_string(song.track()) + ". " : "") + song.PrettyTitle();
    GtkWidget *label = gtk_label_new(text.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 24);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 4);
    gtk_widget_set_margin_bottom(label, 4);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "row-data", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    gtk_list_box_append(GTK_LIST_BOX(collection_list_), row);
  }
  gtk_label_set_text(GTK_LABEL(status_label_), (std::to_string(songs.size()) + " songs").c_str());
}

void MainWindow::RefreshPlaylist() {
  ClearList(playlist_list_);
  Playlist *playlist = app_->playlist_manager()->active();
  if (!playlist) {
    return;
  }
  int index = 0;
  for (const Song &song : playlist->songs()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(song.PrettyTitleWithArtist().c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data(G_OBJECT(row), "row-index", GINT_TO_POINTER(index++));
    gtk_list_box_append(GTK_LIST_BOX(playlist_list_), row);
  }
}

void MainWindow::RefreshPlaylistsList() {
  ClearList(playlists_list_);
  for (const auto &playlist : app_->playlist_manager()->playlists()) {
    AppendStringRow(GTK_LIST_BOX(playlists_list_), playlist->name(), nullptr);
  }
  ClearList(smart_list_);
  AppendStringRow(GTK_LIST_BOX(smart_list_), "All songs", nullptr);
  AppendStringRow(GTK_LIST_BOX(smart_list_), "Never played", nullptr);
  AppendStringRow(GTK_LIST_BOX(smart_list_), "Highest rated", nullptr);
  AppendStringRow(GTK_LIST_BOX(smart_list_), "Newest", nullptr);
}

void MainWindow::RefreshRadio() {
  ClearList(radio_list_);
  for (const RadioChannel &channel : app_->radio_services()->channels()) {
    AppendStringRow(GTK_LIST_BOX(radio_list_), channel.name.empty() ? channel.url : channel.name, nullptr);
  }
  if (app_->radio_services()->channels().empty()) {
    AppendStringRow(GTK_LIST_BOX(radio_list_), "Radio Paradise", nullptr);
    AppendStringRow(GTK_LIST_BOX(radio_list_), "SomaFM", nullptr);
    AppendStringRow(GTK_LIST_BOX(radio_list_), "Radio Browser", nullptr);
  }
  ClearList(streaming_list_);
  for (StreamingService *service : app_->streaming_services()->All()) {
    AppendStringRow(GTK_LIST_BOX(streaming_list_), service->name(), nullptr);
  }
#ifdef HAVE_SUBSONIC
#else
  if (app_->streaming_services()->All().empty()) {
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Subsonic", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Tidal", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Spotify", nullptr);
    AppendStringRow(GTK_LIST_BOX(streaming_list_), "Qobuz", nullptr);
  }
#endif
}

void MainWindow::RefreshDevices() {
  ClearList(devices_list_);
  for (const ConnectedDevice &device : app_->device_manager()->devices()) {
    AppendStringRow(GTK_LIST_BOX(devices_list_), device.friendly_name, nullptr);
  }
  if (app_->device_manager()->devices().empty()) {
    AppendStringRow(GTK_LIST_BOX(devices_list_), "No devices found", nullptr);
  }
}

void MainWindow::UpdateNowPlaying() {
  const Song song = app_->player()->current_song();
  gtk_label_set_text(GTK_LABEL(title_label_), song.PrettyTitle().c_str());
  gtk_label_set_text(GTK_LABEL(artist_label_), song.EffectiveAlbumartist().c_str());
  RefreshPlaylist();
  app_->lyrics_providers()->Fetch(song, [this](const std::string &lyrics, const std::string &) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(lyrics_view_));
    gtk_text_buffer_set_text(buffer, lyrics.empty() ? "No lyrics" : lyrics.c_str(), -1);
  });
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
