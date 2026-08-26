#include "covermanager/albumcovermanager.h"

#include "core/application.h"
#include "covermanager/albumcoverbatch.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "covermanager/albumcovermanagerselection.h"
#include "covermanager/albumcoverexportdialog.h"
#include "covermanager/albumcovermanagerlist.h"
#include "covermanager/coverfromurldialog.h"
#include "covermanager/coverproviders.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <memory>

using DialogHelpers::SetImageFromBytes;

namespace {

struct CoverManagerState {
  Application *app = nullptr;
  GtkWindow *parent = nullptr;
  AlbumCoverChoiceController *covers = nullptr;
  AlbumCoverManagerList catalog;
  AlbumCoverBatch batch;
  GtkWidget *artist_list = nullptr;
  GtkWidget *flow = nullptr;
  GtkWidget *filter = nullptr;
  GtkWidget *hide = nullptr;
  GtkWidget *status = nullptr;
  GtkWidget *progress = nullptr;
  GtkWidget *abort = nullptr;
  GtkWidget *fetch_missing = nullptr;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
  std::string artist_filter;
};

void RebuildAlbums(CoverManagerState *state);
void UpdateBatchUi(CoverManagerState *state);
void FinishBatch(CoverManagerState *state);
void PumpBatch(CoverManagerState *state);
AlbumCoverManagerList::HideCovers HideMode(GtkWidget *combo);

void UpdateBatchUi(CoverManagerState *state) {
  if (!state) {
    return;
  }
  if (state->status) {
    gtk_label_set_text(GTK_LABEL(state->status), state->batch.StatusText().c_str());
  }
  if (state->progress) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), state->batch.Progress());
    gtk_widget_set_visible(state->progress, state->batch.started() && !state->batch.finished());
  }
  if (state->abort) {
    gtk_widget_set_visible(state->abort, state->batch.running());
  }
  if (state->fetch_missing) {
    gtk_widget_set_sensitive(state->fetch_missing, !state->batch.running());
  }
}

void FinishBatch(CoverManagerState *state) {
  if (!state) {
    return;
  }
  UpdateBatchUi(state);
  RebuildAlbums(state);
}

void PumpBatch(CoverManagerState *state) {
  if (!state || !state->alive || !*state->alive) {
    return;
  }
  if (state->batch.cancelled() || !state->batch.Current()) {
    FinishBatch(state);
    return;
  }
  const AlbumCoverBatch::Job *job = state->batch.Current();
  const std::string artist = job->artist;
  const std::string album = job->album;
  Song song = job->song;
  UpdateBatchUi(state);
  state->covers->FetchCover(&song, nullptr, nullptr, [state, artist, album](bool ok) {
    if (!state->alive || !*state->alive) {
      return;
    }
    if (state->batch.cancelled()) {
      FinishBatch(state);
      return;
    }
    if (ok) {
      state->catalog.SetCoverFlag(artist, album, true);
      state->batch.MarkSuccess();
    } else {
      state->batch.MarkFailure();
    }
    PumpBatch(state);
  });
}

std::vector<AlbumCoverManagerList::Album> SelectedAlbums(CoverManagerState *state) {
  std::vector<AlbumCoverManagerList::Album> albums;
  if (!state || !state->flow) {
    return albums;
  }
  GList *children = gtk_flow_box_get_selected_children(GTK_FLOW_BOX(state->flow));
  for (GList *item = children; item; item = item->next) {
    GtkWidget *child = GTK_WIDGET(item->data);
    GtkWidget *card = GTK_IS_FLOW_BOX_CHILD(child) ? gtk_flow_box_child_get_child(GTK_FLOW_BOX_CHILD(child)) : child;
    if (auto *album = static_cast<AlbumCoverManagerList::Album *>(g_object_get_data(G_OBJECT(card), "album"))) {
      albums.push_back(*album);
    }
  }
  g_list_free(children);
  return albums;
}

std::vector<AlbumCoverManagerList::Album> VisibleAlbums(CoverManagerState *state) {
  if (!state) {
    return {};
  }
  const char *filter_text = state->filter ? gtk_editable_get_text(GTK_EDITABLE(state->filter)) : "";
  return state->catalog.Filtered(state->artist_filter, HideMode(state->hide), filter_text ? filter_text : "");
}

std::vector<AlbumCoverManagerList::Album> AlbumsForAction(CoverManagerState *state) {
  const auto selected = SelectedAlbums(state);
  if (AlbumCoverManagerSelection::PreferSelection(selected.size())) {
    return selected;
  }
  return VisibleAlbums(state);
}

void UpdateAlbumStatus(CoverManagerState *state) {
  if (!state || !state->status) {
    return;
  }
  const auto albums = VisibleAlbums(state);
  size_t with_cover = 0;
  for (const auto &album : albums) {
    if (album.has_cover) {
      ++with_cover;
    }
  }
  gtk_label_set_text(GTK_LABEL(state->status),
                     AlbumCoverManagerSelection::StatusText(albums.size(), with_cover, SelectedAlbums(state).size()).c_str());
}

void AddAlbumToPlaylist(CoverManagerState *state, const AlbumCoverManagerList::Album &album, bool replace) {
  if (!state || !state->app) {
    return;
  }
  const SongList songs = AlbumCoverManagerList::SongsInAlbum(state->app->collection()->Songs(), album);
  if (songs.empty()) {
    return;
  }
  if (replace) {
    state->app->playlist_manager()->ClearCurrent();
  }
  state->app->playlist_manager()->AppendSongs(songs);
}

AlbumCoverManagerList::HideCovers HideMode(GtkWidget *combo) {
  if (!GTK_IS_DROP_DOWN(combo)) {
    return AlbumCoverManagerList::HideCovers::None;
  }
  switch (gtk_drop_down_get_selected(GTK_DROP_DOWN(combo))) {
    case 1:
      return AlbumCoverManagerList::HideCovers::WithoutCovers;
    case 2:
      return AlbumCoverManagerList::HideCovers::WithCovers;
    default:
      return AlbumCoverManagerList::HideCovers::None;
  }
}

void ShowAlbumMenu(CoverManagerState *state, AlbumCoverManagerList::Album album, GtkWidget *image) {
  if (!state || !state->covers) {
    return;
  }
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::CStr("Fetch cover"), "cover.fetch");
  g_menu_append(menu, Translations::CStr("Load from file…"), "cover.file");
  g_menu_append(menu, Translations::CStr("Load from URL…"), "cover.url");
  g_menu_append(menu, Translations::CStr("Show cover"), "cover.show");
  g_menu_append(menu, Translations::CStr("Unset cover"), "cover.unset");
  g_menu_append(menu, Translations::CStr("Clear cover"), "cover.clear");
  g_menu_append(menu, Translations::CStr("Delete cover"), "cover.delete");
  g_menu_append(menu, Translations::CStr("Add to playlist"), "cover.append");
  g_menu_append(menu, Translations::CStr("Load to playlist"), "cover.load");
  g_menu_append(menu, Translations::CStr("Save cover to file…"), "cover.save");
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, image);
  GSimpleActionGroup *group = g_simple_action_group_new();
  auto *owned = new AlbumCoverManagerList::Album(std::move(album));
  auto add = [&](const char *name) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_object_set_data(G_OBJECT(action), "album", owned);
    g_object_set_data(G_OBJECT(action), "image", image);
    g_signal_connect(action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer data) {
                       auto *self = static_cast<CoverManagerState *>(data);
                       auto *entry = static_cast<AlbumCoverManagerList::Album *>(g_object_get_data(G_OBJECT(act), "album"));
                       GtkWidget *image_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(act), "image"));
                       if (!self || !entry || !self->covers) {
                         return;
                       }
                       const char *name = g_action_get_name(G_ACTION(act));
                       if (g_strcmp0(name, "fetch") == 0) {
                         const std::string artist = entry->artist;
                         const std::string album = entry->album;
                         self->covers->FetchCover(&entry->song, image_widget, nullptr, [self, artist, album](bool ok) {
                           if (!self->alive || !*self->alive) {
                             return;
                           }
                           if (ok) {
                             self->catalog.SetCoverFlag(artist, album, true);
                           }
                           RebuildAlbums(self);
                         });
                         return;
                       } else if (g_strcmp0(name, "file") == 0) {
                         self->covers->LoadCoverFromFile(self->parent, &entry->song, image_widget);
                       } else if (g_strcmp0(name, "url") == 0) {
                         self->covers->LoadCoverFromURL(self->parent, &entry->song, image_widget);
                       } else if (g_strcmp0(name, "show") == 0) {
                         self->covers->ShowCover(self->parent, entry->song);
                       } else if (g_strcmp0(name, "unset") == 0) {
                         self->covers->UnsetCover(&entry->song, image_widget);
                         self->catalog.SetCoverFlag(entry->artist, entry->album, false);
                       } else if (g_strcmp0(name, "clear") == 0) {
                         self->covers->ClearCover(&entry->song, image_widget);
                       } else if (g_strcmp0(name, "delete") == 0) {
                         self->covers->DeleteCover(&entry->song, image_widget);
                         self->catalog.SetCoverFlag(entry->artist, entry->album, false);
                       } else if (g_strcmp0(name, "append") == 0) {
                         AddAlbumToPlaylist(self, *entry, false);
                       } else if (g_strcmp0(name, "load") == 0) {
                         AddAlbumToPlaylist(self, *entry, true);
                       } else if (g_strcmp0(name, "save") == 0) {
                         self->covers->SaveCoverToFile(self->parent, entry->song);
                       }
                       RebuildAlbums(self);
                     }),
                     state);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  };
  add("fetch");
  add("file");
  add("url");
  add("show");
  add("unset");
  add("clear");
  add("delete");
  add("append");
  add("load");
  add("save");
  gtk_widget_insert_action_group(popover, "cover", G_ACTION_GROUP(group));
  g_object_set_data_full(G_OBJECT(popover), "album", owned, [](gpointer p) { delete static_cast<AlbumCoverManagerList::Album *>(p); });
  gtk_popover_popup(GTK_POPOVER(popover));
}

void RebuildAlbums(CoverManagerState *state) {
  if (!state || !state->flow) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(state->flow);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(state->flow), child);
    child = next;
  }
  const char *filter_text = state->filter ? gtk_editable_get_text(GTK_EDITABLE(state->filter)) : "";
  const auto albums = state->catalog.Filtered(state->artist_filter, HideMode(state->hide), filter_text ? filter_text : "");
  for (const auto &album : albums) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(card, 140, -1);
    GtkWidget *image = gtk_image_new();
    const auto cover = state->app->albumcover_loader()->LoadData(album.song);
    SetImageFromBytes(image, cover, 120);
    GtkWidget *title = gtk_label_new(album.album.c_str());
    gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
    GtkWidget *artist = gtk_label_new(album.artist.c_str());
    gtk_widget_add_css_class(artist, "dim-label");
    gtk_label_set_ellipsize(GTK_LABEL(artist), PANGO_ELLIPSIZE_END);
    GtkWidget *button = gtk_button_new_with_label(album.has_cover ? Translations::CStr("Replace") : Translations::CStr("Fetch"));
    gtk_widget_add_css_class(button, "flat");
    auto *owned = new AlbumCoverManagerList::Album(album);
    g_object_set_data_full(G_OBJECT(card), "album", owned, [](gpointer p) { delete static_cast<AlbumCoverManagerList::Album *>(p); });
    g_object_set_data(G_OBJECT(button), "album", owned);
    g_object_set_data(G_OBJECT(button), "image", image);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<CoverManagerState *>(data);
                       auto *entry = static_cast<AlbumCoverManagerList::Album *>(g_object_get_data(G_OBJECT(btn), "album"));
                       GtkWidget *image_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "image"));
                       if (!self || !entry || !self->covers) {
                         return;
                       }
                       self->covers->FetchCover(&entry->song, image_widget, GTK_WIDGET(btn));
                       self->catalog.SetCoverFlag(entry->artist, entry->album, true);
                     }),
                     state);
    GtkGesture *menu = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
    gtk_widget_add_controller(card, GTK_EVENT_CONTROLLER(menu));
    g_object_set_data(G_OBJECT(menu), "album", owned);
    g_object_set_data(G_OBJECT(menu), "image", image);
    g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble, gpointer data) {
                       auto *self = static_cast<CoverManagerState *>(data);
                       auto *entry = static_cast<AlbumCoverManagerList::Album *>(g_object_get_data(G_OBJECT(click), "album"));
                       GtkWidget *image_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(click), "image"));
                       if (self && entry) {
                         ShowAlbumMenu(self, *entry, image_widget);
                       }
                     }),
                     state);
    GtkGesture *activate = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(activate), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(card, GTK_EVENT_CONTROLLER(activate));
    g_object_set_data(G_OBJECT(activate), "album", owned);
    g_signal_connect(activate, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint n_press, gdouble, gdouble, gpointer data) {
                       if (n_press != 2) {
                         return;
                       }
                       auto *self = static_cast<CoverManagerState *>(data);
                       auto *entry = static_cast<AlbumCoverManagerList::Album *>(g_object_get_data(G_OBJECT(click), "album"));
                       if (self && entry) {
                         AddAlbumToPlaylist(self, *entry, false);
                       }
                     }),
                     state);
    gtk_box_append(GTK_BOX(card), image);
    gtk_box_append(GTK_BOX(card), title);
    gtk_box_append(GTK_BOX(card), artist);
    gtk_box_append(GTK_BOX(card), button);
    gtk_flow_box_append(GTK_FLOW_BOX(state->flow), card);
  }
  UpdateAlbumStatus(state);
}

void RebuildArtists(CoverManagerState *state) {
  if (!state || !state->artist_list) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(state->artist_list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(state->artist_list), child);
    child = next;
  }
  auto add_row = [&](const char *label, const char *id) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *text = gtk_label_new(label);
    gtk_widget_set_halign(text, GTK_ALIGN_START);
    gtk_widget_set_margin_start(text, 8);
    gtk_widget_set_margin_end(text, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), text);
    g_object_set_data_full(G_OBJECT(row), "artist-id", g_strdup(id), g_free);
    gtk_list_box_append(GTK_LIST_BOX(state->artist_list), row);
  };
  add_row(Translations::CStr("All artists"), AlbumCoverManagerList::kAllArtists);
  for (const std::string &artist : state->catalog.Artists()) {
    add_row(artist.c_str(), artist.c_str());
  }
}

}  // namespace

void AlbumCoverManager::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Cover manager"));
  adw_dialog_set_content_width(dialog, 860);
  adw_dialog_set_content_height(dialog, 680);

  auto *state = new CoverManagerState;
  state->app = app;
  state->parent = parent;
  state->covers = new AlbumCoverChoiceController(app);
  state->catalog.SetSongs(app->collection()->Songs());
  for (const auto &album : state->catalog.albums()) {
    if (!app->albumcover_loader()->LoadData(album.song).empty()) {
      state->catalog.SetCoverFlag(album.artist, album.album, true);
    }
  }
  g_object_set_data_full(G_OBJECT(dialog), "state", state, [](gpointer p) {
    auto *self = static_cast<CoverManagerState *>(p);
    self->batch.Cancel();
    if (self->alive) {
      *self->alive = false;
    }
    delete self->covers;
    delete self;
  });

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  state->filter = gtk_search_entry_new();
  gtk_widget_set_hexpand(state->filter, TRUE);
  gtk_box_append(GTK_BOX(toolbar), state->filter);
  const char *hide_labels[] = {Translations::CStr("All albums"), Translations::CStr("Albums with covers"),
                               Translations::CStr("Albums without covers"), nullptr};
  state->hide = gtk_drop_down_new_from_strings(hide_labels);
  gtk_box_append(GTK_BOX(toolbar), state->hide);
  gtk_box_append(GTK_BOX(box), toolbar);

  GtkWidget *split = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *artist_scroll = gtk_scrolled_window_new();
  gtk_widget_set_size_request(artist_scroll, 180, -1);
  gtk_widget_set_vexpand(artist_scroll, TRUE);
  state->artist_list = gtk_list_box_new();
  gtk_widget_add_css_class(state->artist_list, "navigation-sidebar");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(artist_scroll), state->artist_list);
  gtk_box_append(GTK_BOX(split), artist_scroll);

  GtkWidget *album_scroll = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(album_scroll, TRUE);
  gtk_widget_set_vexpand(album_scroll, TRUE);
  state->flow = gtk_flow_box_new();
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(state->flow), 2);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(state->flow), 5);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(state->flow), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(album_scroll), state->flow);
  gtk_box_append(GTK_BOX(split), album_scroll);
  gtk_box_append(GTK_BOX(box), split);

  state->status = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(state->status), 0.0f);
  gtk_box_append(GTK_BOX(box), state->status);
  state->progress = gtk_progress_bar_new();
  gtk_widget_set_visible(state->progress, FALSE);
  gtk_box_append(GTK_BOX(box), state->progress);

  GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *fetch_missing = gtk_button_new_with_label(Translations::CStr("Fetch all missing"));
  gtk_widget_add_css_class(fetch_missing, "suggested-action");
  state->fetch_missing = fetch_missing;
  state->abort = gtk_button_new_with_label(Translations::CStr("Abort"));
  gtk_widget_set_visible(state->abort, FALSE);
  GtkWidget *add_playlist = gtk_button_new_with_label(Translations::CStr("Add to playlist"));
  GtkWidget *load_playlist = gtk_button_new_with_label(Translations::CStr("Load to playlist"));
  GtkWidget *from_url = gtk_button_new_with_label(Translations::CStr("Load cover from URL…"));
  GtkWidget *export_btn = gtk_button_new_with_label(Translations::CStr("Export covers…"));
  GtkWidget *stats = gtk_button_new_with_label(Translations::CStr("Statistics"));
  gtk_box_append(GTK_BOX(actions), fetch_missing);
  gtk_box_append(GTK_BOX(actions), state->abort);
  gtk_box_append(GTK_BOX(actions), add_playlist);
  gtk_box_append(GTK_BOX(actions), load_playlist);
  gtk_box_append(GTK_BOX(actions), from_url);
  gtk_box_append(GTK_BOX(actions), export_btn);
  gtk_box_append(GTK_BOX(actions), stats);
  gtk_box_append(GTK_BOX(box), actions);

  auto refresh = +[](GtkWidget *, gpointer data) { RebuildAlbums(static_cast<CoverManagerState *>(data)); };
  g_signal_connect(state->filter, "search-changed", G_CALLBACK(refresh), state);
  g_signal_connect(state->hide, "notify::selected", G_CALLBACK(+[](GObject *, GParamSpec *, gpointer data) {
                     RebuildAlbums(static_cast<CoverManagerState *>(data));
                   }),
                   state);
  g_signal_connect(state->artist_list, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "artist-id"));
                     self->artist_filter = id ? id : "";
                     RebuildAlbums(self);
                   }),
                   state);

  g_signal_connect(fetch_missing, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     if (self->batch.running()) {
                       return;
                     }
                     self->batch.Reset();
                     for (const auto &album : AlbumsForAction(self)) {
                       if (album.has_cover) {
                         continue;
                       }
                       AlbumCoverBatch::Job job;
                       job.artist = album.artist;
                       job.album = album.album;
                       job.song = album.song;
                       self->batch.Enqueue(std::move(job));
                     }
                     self->batch.Start();
                     PumpBatch(self);
                   }),
                   state);
  g_signal_connect(state->abort, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     self->batch.Cancel();
                     FinishBatch(self);
                   }),
                   state);
  g_signal_connect(add_playlist, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     for (const auto &album : AlbumsForAction(self)) {
                       AddAlbumToPlaylist(self, album, false);
                     }
                   }),
                   state);
  g_signal_connect(load_playlist, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     bool first = true;
                     for (const auto &album : AlbumsForAction(self)) {
                       AddAlbumToPlaylist(self, album, first);
                       first = false;
                     }
                   }),
                   state);
  g_signal_connect(state->flow, "selected-children-changed", G_CALLBACK(+[](GtkFlowBox *, gpointer data) {
                     UpdateAlbumStatus(static_cast<CoverManagerState *>(data));
                   }),
                   state);
  g_signal_connect(from_url, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     CoverFromUrlDialog::Show(self->parent, self->app);
                   }),
                   state);
  g_signal_connect(export_btn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     AlbumCoverExportDialog::Show(self->parent, self->app);
                   }),
                   state);
  g_signal_connect(stats, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     self->covers->ShowStatistics(self->parent);
                   }),
                   state);

  RebuildArtists(state);
  RebuildAlbums(state);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
