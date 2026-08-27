#include "covermanager/albumcovermanager.h"

#include "constants/covermanagersettings.h"
#include "core/application.h"
#include "dialogs/dialoggeometry.h"
#include "covermanager/albumcoverbatch.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "covermanager/albumcovermanagerselection.h"
#include "covermanager/albumcoverexportdialog.h"
#include "covermanager/albumcovermanagerlist.h"
#include "covermanager/covermanagerexportscope.h"
#include "covermanager/covermanageractions.h"
#include "covermanager/covermanagerstats.h"
#include "covermanager/covermanagerview.h"
#include "covermanager/covermanageractivate.h"
#include "covermanager/covermanagermenu.h"
#include "covermanager/coverproviders.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <algorithm>
#include <memory>
#include <vector>

using DialogHelpers::SetImageFromBytes;

namespace {

struct CoverManagerState {
  Application *app = nullptr;
  GtkWindow *parent = nullptr;
  AdwDialog *dialog = nullptr;
  AlbumCoverChoiceController *covers = nullptr;
  AlbumCoverManagerList catalog;
  AlbumCoverBatch batch;
  GtkWidget *artist_list = nullptr;
  GtkWidget *flow = nullptr;
  GtkWidget *filter = nullptr;
  GtkWidget *view = nullptr;
  int hide_index = 0;
  GtkWidget *total_albums = nullptr;
  GtkWidget *without_cover = nullptr;
  GtkWidget *status = nullptr;
  GtkWidget *progress = nullptr;
  GtkWidget *abort = nullptr;
  GtkWidget *fetch_missing = nullptr;
  GtkWidget *export_btn = nullptr;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
  std::string artist_filter;
  std::string typeahead;
  guint typeahead_timeout = 0;
};

void RebuildAlbums(CoverManagerState *state);
void ApplySearchPlaceholder(GtkWidget *widget, const char *text);
void UpdateBatchUi(CoverManagerState *state);
void FinishBatch(CoverManagerState *state);
void PumpBatch(CoverManagerState *state);
AlbumCoverManagerList::HideCovers HideMode(CoverManagerState *state);
void ResetTypeAhead(CoverManagerState *state);
void AppendTypeAhead(CoverManagerState *state, gunichar ch);
void SelectArtistRow(CoverManagerState *state, int index);
void SelectAlbumChild(CoverManagerState *state, int index);
int SelectedArtistIndex(CoverManagerState *state);
int SelectedAlbumIndex(CoverManagerState *state);
int ArtistCount(CoverManagerState *state);
int AlbumCount(CoverManagerState *state);
int FlowColumns(CoverManagerState *state);
std::vector<std::string> ArtistLabels(CoverManagerState *state);
std::vector<std::string> AlbumLabels(CoverManagerState *state);
gboolean OnCoverKey(CoverManagerState *state, guint keyval, GdkModifierType modifiers, bool albums);
void ShowAlbumMenu(CoverManagerState *state, AlbumCoverManagerList::Album album, GtkWidget *image);
GtkWidget *SelectedAlbumImage(CoverManagerState *state);

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
    const bool has_providers =
        state->app && state->app->cover_providers() && CoverManagerMenu::HasAnyProviders(state->app->cover_providers()->All().size());
    gtk_widget_set_sensitive(state->fetch_missing, CoverManagerActions::FetchEnabled(state->batch.running(), has_providers));
  }
  if (state->export_btn) {
    gtk_widget_set_sensitive(state->export_btn, CoverManagerActions::ExportEnabled(state->batch.running()));
  }
  if (state->dialog) {
    adw_dialog_set_can_close(state->dialog, CoverManagerActions::CanCloseWithoutConfirm(state->batch.running()) ? TRUE : FALSE);
  }
}

void FinishBatch(CoverManagerState *state) {
  if (!state) {
    return;
  }
  UpdateBatchUi(state);
  RebuildAlbums(state);
  if (state->covers && CoverManagerActions::ShowStatisticsWhenFetchFinishes(state->batch.started(), state->batch.cancelled(), state->batch.total())) {
    state->covers->ShowStatistics(state->parent);
  }
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
  return state->catalog.Filtered(state->artist_filter, HideMode(state), filter_text ? filter_text : "");
}

std::vector<AlbumCoverManagerList::Album> AlbumsForAction(CoverManagerState *state) {
  const auto selected = SelectedAlbums(state);
  if (AlbumCoverManagerSelection::PreferSelection(selected.size())) {
    return selected;
  }
  return VisibleAlbums(state);
}

void UpdateAlbumStatus(CoverManagerState *state) {
  if (!state) {
    return;
  }
  const auto albums = VisibleAlbums(state);
  int with_cover = 0;
  for (const auto &album : albums) {
    if (album.has_cover) {
      ++with_cover;
    }
  }
  const int total = static_cast<int>(albums.size());
  if (state->total_albums) {
    gtk_label_set_text(GTK_LABEL(state->total_albums), CoverManagerStats::CountText(total).c_str());
  }
  if (state->without_cover) {
    gtk_label_set_text(GTK_LABEL(state->without_cover), CoverManagerStats::CountText(CoverManagerStats::WithoutCover(total, with_cover)).c_str());
  }
  if (state->status) {
    gtk_label_set_text(GTK_LABEL(state->status),
                       AlbumCoverManagerSelection::StatusText(albums.size(), static_cast<size_t>(with_cover), SelectedAlbums(state).size()).c_str());
  }
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

AlbumCoverManagerList::HideCovers HideMode(CoverManagerState *state) {
  return CoverManagerView::HideFromIndex(state ? state->hide_index : 0);
}

void ApplySearchPlaceholder(GtkWidget *widget, const char *text) {
  if (!widget || !text) {
    return;
  }
  if (GTK_IS_TEXT(widget)) {
    gtk_text_set_placeholder_text(GTK_TEXT(widget), text);
    return;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(widget); child; child = gtk_widget_get_next_sibling(child)) {
    ApplySearchPlaceholder(child, text);
  }
}

void ResetTypeAhead(CoverManagerState *state) {
  if (!state) {
    return;
  }
  state->typeahead.clear();
  if (state->typeahead_timeout) {
    g_source_remove(state->typeahead_timeout);
    state->typeahead_timeout = 0;
  }
}

void AppendTypeAhead(CoverManagerState *state, gunichar ch) {
  if (!state) {
    return;
  }
  gchar utf8[8] = {};
  const gint len = g_unichar_to_utf8(ch, utf8);
  state->typeahead.append(utf8, static_cast<size_t>(len));
  if (state->typeahead_timeout) {
    g_source_remove(state->typeahead_timeout);
  }
  state->typeahead_timeout = g_timeout_add(1000, [](gpointer data) -> gboolean {
    auto *self = static_cast<CoverManagerState *>(data);
    self->typeahead_timeout = 0;
    self->typeahead.clear();
    return G_SOURCE_REMOVE;
  }, state);
}

int ArtistCount(CoverManagerState *state) {
  int count = 0;
  if (!state || !state->artist_list) {
    return count;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(state->artist_list); child; child = gtk_widget_get_next_sibling(child)) {
    if (GTK_IS_LIST_BOX_ROW(child)) {
      ++count;
    }
  }
  return count;
}

int AlbumCount(CoverManagerState *state) {
  int count = 0;
  if (!state || !state->flow) {
    return count;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(state->flow); child; child = gtk_widget_get_next_sibling(child)) {
    ++count;
  }
  return count;
}

int SelectedArtistIndex(CoverManagerState *state) {
  if (!state || !state->artist_list) {
    return -1;
  }
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->artist_list));
  return row ? gtk_list_box_row_get_index(row) : -1;
}

int SelectedAlbumIndex(CoverManagerState *state) {
  if (!state || !state->flow) {
    return -1;
  }
  GList *selected = gtk_flow_box_get_selected_children(GTK_FLOW_BOX(state->flow));
  if (!selected) {
    return -1;
  }
  int index = -1;
  if (GTK_IS_FLOW_BOX_CHILD(selected->data)) {
    index = gtk_flow_box_child_get_index(GTK_FLOW_BOX_CHILD(selected->data));
  }
  g_list_free(selected);
  return index;
}

GtkWidget *SelectedAlbumImage(CoverManagerState *state) {
  if (!state || !state->flow) {
    return nullptr;
  }
  GList *selected = gtk_flow_box_get_selected_children(GTK_FLOW_BOX(state->flow));
  if (!selected) {
    return nullptr;
  }
  GtkWidget *child = GTK_WIDGET(selected->data);
  GtkWidget *card = GTK_IS_FLOW_BOX_CHILD(child) ? gtk_flow_box_child_get_child(GTK_FLOW_BOX_CHILD(child)) : child;
  g_list_free(selected);
  return card ? GTK_WIDGET(g_object_get_data(G_OBJECT(card), "image")) : nullptr;
}

void SelectArtistRow(CoverManagerState *state, int index) {
  if (!state || !state->artist_list || index < 0) {
    return;
  }
  GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->artist_list), index);
  if (!row) {
    return;
  }
  gtk_list_box_select_row(GTK_LIST_BOX(state->artist_list), row);
  gtk_widget_grab_focus(GTK_WIDGET(row));
  const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "artist-id"));
  state->artist_filter = id ? id : "";
  RebuildAlbums(state);
}

void SelectAlbumChild(CoverManagerState *state, int index) {
  if (!state || !state->flow || index < 0) {
    return;
  }
  GtkFlowBoxChild *child = gtk_flow_box_get_child_at_index(GTK_FLOW_BOX(state->flow), index);
  if (!child) {
    return;
  }
  gtk_flow_box_unselect_all(GTK_FLOW_BOX(state->flow));
  gtk_flow_box_select_child(GTK_FLOW_BOX(state->flow), child);
  gtk_widget_grab_focus(GTK_WIDGET(child));
}

int FlowColumns(CoverManagerState *state) {
  if (!state || !state->flow) {
    return 1;
  }
  const int max = static_cast<int>(gtk_flow_box_get_max_children_per_line(GTK_FLOW_BOX(state->flow)));
  GtkWidget *first = gtk_widget_get_first_child(state->flow);
  if (!first) {
    return std::max(1, max);
  }
  const int width = gtk_widget_get_width(state->flow);
  const int child_w = gtk_widget_get_width(first);
  if (width <= 0 || child_w <= 0) {
    return std::max(1, max);
  }
  return std::max(1, std::min(max, width / child_w));
}

std::vector<std::string> ArtistLabels(CoverManagerState *state) {
  std::vector<std::string> labels;
  if (!state || !state->artist_list) {
    return labels;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(state->artist_list); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    GtkWidget *label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(child));
    labels.push_back(GTK_IS_LABEL(label) ? gtk_label_get_text(GTK_LABEL(label)) : "");
  }
  return labels;
}

std::vector<std::string> AlbumLabels(CoverManagerState *state) {
  std::vector<std::string> labels;
  if (!state) {
    return labels;
  }
  for (const auto &album : VisibleAlbums(state)) {
    labels.push_back(album.album);
  }
  return labels;
}

gboolean OnCoverKey(CoverManagerState *state, guint keyval, GdkModifierType modifiers, bool albums) {
  if (!state) {
    return FALSE;
  }
  if (keyval == GDK_KEY_Escape) {
    ResetTypeAhead(state);
    return TRUE;
  }
  if (CoverManagerMenu::IsKeyboardTrigger(keyval, static_cast<unsigned>(modifiers))) {
    if (CoverManagerMenu::ShouldShowMenu(albums, !SelectedAlbums(state).empty())) {
      GtkWidget *image = SelectedAlbumImage(state);
      const auto selected = SelectedAlbums(state);
      if (image && !selected.empty()) {
        ShowAlbumMenu(state, selected.front(), image);
      }
    }
    return TRUE;
  }
  if (CoverManagerActivate::IsEnter(keyval)) {
    if (albums) {
      const auto selected = SelectedAlbums(state);
      if (!selected.empty() && state->covers && CoverManagerActivate::ForAlbumEnter() == CoverManagerActivate::Action::ShowCover) {
        state->covers->ShowCover(state->parent, selected.front().song);
      }
      return TRUE;
    }
    if (SelectedArtistIndex(state) >= 0) {
      return TRUE;
    }
  }
  int horizontal = 0;
  int vertical = 0;
  if (keyval == GDK_KEY_Right) {
    horizontal = 1;
  } else if (keyval == GDK_KEY_Left) {
    horizontal = -1;
  } else if (keyval == GDK_KEY_Down) {
    vertical = 1;
  } else if (keyval == GDK_KEY_Up) {
    vertical = -1;
  }
  if (horizontal != 0 || vertical != 0) {
    if (albums) {
      const int count = AlbumCount(state);
      const int current = std::max(0, SelectedAlbumIndex(state));
      SelectAlbumChild(state, AlbumCoverManagerSelection::WrapIndex(current, count, AlbumCoverManagerSelection::FlowDelta(FlowColumns(state), horizontal, vertical)));
    } else if (horizontal == 0) {
      const int count = ArtistCount(state);
      const int current = std::max(0, SelectedArtistIndex(state));
      SelectArtistRow(state, AlbumCoverManagerSelection::WrapIndex(current, count, vertical));
    }
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch && g_unichar_isprint(ch)) {
    AppendTypeAhead(state, ch);
    const int index = AlbumCoverManagerSelection::FirstPrefixIndex(albums ? AlbumLabels(state) : ArtistLabels(state), state->typeahead);
    if (index >= 0) {
      if (albums) {
        SelectAlbumChild(state, index);
      } else {
        SelectArtistRow(state, index);
      }
    }
    return TRUE;
  }
  return FALSE;
}

void ShowAlbumMenu(CoverManagerState *state, AlbumCoverManagerList::Album album, GtkWidget *image) {
  if (!state || !state->covers) {
    return;
  }
  const bool has_providers =
      state->app && state->app->cover_providers() && CoverManagerMenu::HasAnyProviders(state->app->cover_providers()->All().size());
  const CoverManagerMenu::CoverState cover_state = CoverManagerMenu::FromSong(album.song, has_providers, album.has_cover);
  GMenu *menu = g_menu_new();
  for (const CoverChoiceMenu::Item &item : CoverManagerMenu::VisibleCoverItems(cover_state)) {
    g_menu_append(menu, Translations::CStr(item.label), CoverChoiceMenu::ActionPath("cover", item.id).c_str());
  }
  if (CoverManagerMenu::IncludePlaylistItems(cover_state)) {
    GMenu *playlist = g_menu_new();
    for (const CoverManagerMenu::Extra &item : CoverManagerMenu::PlaylistItems()) {
      g_menu_append(playlist, Translations::CStr(item.label), CoverChoiceMenu::ActionPath("cover", item.id).c_str());
    }
    g_menu_append_section(menu, nullptr, G_MENU_MODEL(playlist));
  }
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
                       }
                       if (g_strcmp0(name, "search") == 0) {
                         const std::string artist = entry->artist;
                         const std::string album = entry->album;
                         self->covers->SearchForCover(self->parent, entry->song, [self, artist, album](bool ok) {
                           if (!self->alive || !*self->alive) {
                             return;
                           }
                           if (ok) {
                             self->catalog.SetCoverFlag(artist, album, true);
                           }
                           RebuildAlbums(self);
                         });
                         return;
                       }
                       if (CoverManagerMenu::IsPlaylistId(name)) {
                         AddAlbumToPlaylist(self, *entry, CoverManagerMenu::LoadReplacesPlaylist(name));
                       } else if (CoverManagerMenu::IsCoverId(name)) {
                         self->covers->Perform(CoverChoiceMenu::FromId(name), self->parent, &entry->song, image_widget);
                         if (g_strcmp0(name, "unset") == 0 || g_strcmp0(name, "delete") == 0) {
                           self->catalog.SetCoverFlag(entry->artist, entry->album, false);
                         }
                       }
                       RebuildAlbums(self);
                     }),
                     state);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
  };
  for (const CoverChoiceMenu::Item &item : CoverManagerMenu::CoverItems()) {
    add(item.id);
  }
  for (const CoverManagerMenu::Extra &item : CoverManagerMenu::PlaylistItems()) {
    add(item.id);
  }
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
  const auto albums = state->catalog.Filtered(state->artist_filter, HideMode(state), filter_text ? filter_text : "");
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
    g_object_set_data(G_OBJECT(card), "image", image);
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
                       if (self && entry && self->covers && CoverManagerActions::DoubleClickShowsCover()) {
                         self->covers->ShowCover(self->parent, entry->song);
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
  adw_dialog_set_title(dialog, Translations::CStr(CoverManagerActions::WindowTitle()));
  DialogGeometry::Apply(dialog, CoverManagerSettings::kSettingsGroup, CoverManagerSettings::kGeometry,
                        CoverManagerSettings::kDefaultWidth, CoverManagerSettings::kDefaultHeight);

  auto *state = new CoverManagerState;
  state->app = app;
  state->parent = parent;
  state->dialog = dialog;
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
    if (self->typeahead_timeout) {
      g_source_remove(self->typeahead_timeout);
      self->typeahead_timeout = 0;
    }
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
  ApplySearchPlaceholder(state->filter, Translations::CStr(CoverManagerView::SearchPlaceholder()));
  gtk_box_append(GTK_BOX(toolbar), state->filter);
  state->view = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(state->view), Translations::CStr(CoverManagerView::ButtonLabel()));
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(state->view), CoverManagerView::ButtonIcon());
  gtk_widget_set_tooltip_text(state->view, Translations::CStr(CoverManagerView::ButtonLabel()));
  GMenu *view_menu = g_menu_new();
  for (int i = 0; i < CoverManagerView::kCount; ++i) {
    char action[64];
    g_snprintf(action, sizeof(action), "coverview.hide(%d)", i);
    g_menu_append(view_menu, Translations::CStr(CoverManagerView::kLabels[i]), action);
  }
  GSimpleActionGroup *view_group = g_simple_action_group_new();
  GSimpleAction *hide_action = g_simple_action_new_stateful("hide", G_VARIANT_TYPE_INT32, g_variant_new_int32(state->hide_index));
  g_signal_connect(hide_action, "activate", G_CALLBACK(+[](GSimpleAction *action, GVariant *param, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     g_simple_action_set_state(action, param);
                     self->hide_index = g_variant_get_int32(param);
                     RebuildAlbums(self);
                   }),
                   state);
  g_action_map_add_action(G_ACTION_MAP(view_group), G_ACTION(hide_action));
  gtk_widget_insert_action_group(state->view, "coverview", G_ACTION_GROUP(view_group));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(state->view), G_MENU_MODEL(view_menu));
  g_object_unref(view_group);
  g_object_unref(view_menu);
  gtk_box_append(GTK_BOX(toolbar), state->view);
  gtk_box_append(GTK_BOX(box), toolbar);

  GtkWidget *split = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *artist_scroll = gtk_scrolled_window_new();
  gtk_widget_set_size_request(artist_scroll, 180, -1);
  gtk_widget_set_vexpand(artist_scroll, TRUE);
  state->artist_list = gtk_list_box_new();
  gtk_widget_add_css_class(state->artist_list, "navigation-sidebar");
  gtk_widget_set_focusable(state->artist_list, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(artist_scroll), state->artist_list);
  gtk_box_append(GTK_BOX(split), artist_scroll);

  GtkWidget *album_scroll = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(album_scroll, TRUE);
  gtk_widget_set_vexpand(album_scroll, TRUE);
  state->flow = gtk_flow_box_new();
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(state->flow), 2);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(state->flow), 5);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(state->flow), GTK_SELECTION_MULTIPLE);
  gtk_widget_set_focusable(state->flow, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(album_scroll), state->flow);
  gtk_box_append(GTK_BOX(split), album_scroll);
  gtk_box_append(GTK_BOX(box), split);

  GtkWidget *stats_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
  GtkWidget *total_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *total_caption = gtk_label_new(Translations::CStr(CoverManagerStats::TotalLabel()));
  state->total_albums = gtk_label_new("0");
  gtk_widget_set_halign(state->total_albums, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(total_box), total_caption);
  gtk_box_append(GTK_BOX(total_box), state->total_albums);
  GtkWidget *without_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *without_caption = gtk_label_new(Translations::CStr(CoverManagerStats::WithoutLabel()));
  state->without_cover = gtk_label_new("0");
  gtk_widget_set_halign(state->without_cover, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(without_box), without_caption);
  gtk_box_append(GTK_BOX(without_box), state->without_cover);
  gtk_box_append(GTK_BOX(stats_row), total_box);
  gtk_box_append(GTK_BOX(stats_row), without_box);
  gtk_box_append(GTK_BOX(box), stats_row);

  state->status = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(state->status), 0.0f);
  gtk_box_append(GTK_BOX(box), state->status);
  state->progress = gtk_progress_bar_new();
  gtk_widget_set_visible(state->progress, FALSE);
  gtk_box_append(GTK_BOX(box), state->progress);

  GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *fetch_missing = gtk_button_new_with_label(Translations::CStr(CoverManagerStats::FetchMissing()));
  gtk_widget_add_css_class(fetch_missing, "suggested-action");
  state->fetch_missing = fetch_missing;
  state->abort = gtk_button_new_with_label(Translations::CStr("Abort"));
  gtk_widget_set_visible(state->abort, FALSE);
  state->export_btn = gtk_button_new_with_label(Translations::CStr(CoverManagerStats::Export()));
  gtk_box_append(GTK_BOX(actions), fetch_missing);
  gtk_box_append(GTK_BOX(actions), state->abort);
  gtk_box_append(GTK_BOX(actions), state->export_btn);
  gtk_box_append(GTK_BOX(box), actions);

  auto refresh = +[](GtkWidget *, gpointer data) { RebuildAlbums(static_cast<CoverManagerState *>(data)); };
  g_signal_connect(state->filter, "search-changed", G_CALLBACK(refresh), state);
  g_signal_connect(state->artist_list, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "artist-id"));
                     self->artist_filter = id ? id : "";
                     RebuildAlbums(self);
                   }),
                   state);
  GtkEventController *artist_keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(state->artist_list, artist_keys);
  g_signal_connect(artist_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return OnCoverKey(static_cast<CoverManagerState *>(data), keyval, state, false);
                   })),
                   state);
  GtkEventController *album_keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(state->flow, album_keys);
  g_signal_connect(album_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return OnCoverKey(static_cast<CoverManagerState *>(data), keyval, state, true);
                   })),
                   state);

  g_signal_connect(fetch_missing, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     if (self->batch.running()) {
                       return;
                     }
                     if (self->covers) {
                       self->covers->ResetStatistics();
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
  g_signal_connect(state->flow, "selected-children-changed", G_CALLBACK(+[](GtkFlowBox *, gpointer data) {
                     UpdateAlbumStatus(static_cast<CoverManagerState *>(data));
                   }),
                   state);
  g_signal_connect(state->export_btn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<CoverManagerState *>(data);
                     if (self->batch.running()) {
                       return;
                     }
                     AlbumCoverExportDialog::Show(self->parent, self->app,
                                                   CoverManagerExportScope::SongsToExport(VisibleAlbums(self)));
                   }),
                   state);
  g_signal_connect(dialog, "close-attempt", G_CALLBACK((+[](AdwDialog *self, gpointer data) {
                     auto *state = static_cast<CoverManagerState *>(data);
                     if (!CoverManagerActions::ShouldConfirmCloseOnFetch(state->batch.running())) {
                       adw_dialog_force_close(self);
                       return;
                     }
                     AdwAlertDialog *confirm = ADW_ALERT_DIALOG(adw_alert_dialog_new(
                         Translations::CStr(CoverManagerActions::CloseConfirmTitle()), Translations::CStr(CoverManagerActions::CloseConfirmMessage())));
                     adw_alert_dialog_add_responses(confirm, "keep", Translations::CStr(CoverManagerActions::CloseDontStop()), "abort",
                                                    Translations::CStr(CoverManagerActions::CloseAbort()), nullptr);
                     adw_alert_dialog_set_response_appearance(confirm, "abort", ADW_RESPONSE_DESTRUCTIVE);
                     adw_alert_dialog_set_default_response(confirm, "abort");
                     g_signal_connect(confirm, "response", G_CALLBACK((+[](AdwAlertDialog *, const char *response, gpointer data) {
                                        auto *state = static_cast<CoverManagerState *>(data);
                                        if (g_strcmp0(response, "abort") != 0 || !state->dialog) {
                                          return;
                                        }
                                        state->batch.Cancel();
                                        adw_dialog_set_can_close(state->dialog, TRUE);
                                        adw_dialog_close(state->dialog);
                                      })),
                                      state);
                     adw_dialog_present(ADW_DIALOG(confirm), GTK_WIDGET(self));
                   })),
                   state);

  RebuildArtists(state);
  RebuildAlbums(state);
  SelectArtistRow(state, 0);
  adw_dialog_set_child(dialog, box);
  DialogGeometry::BindClosed(dialog, CoverManagerSettings::kSettingsGroup, CoverManagerSettings::kGeometry);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
