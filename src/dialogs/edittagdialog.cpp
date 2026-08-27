#include "config.h"
#include "dialogs/edittagdialog.h"

#include "constants/edittagdialogsettings.h"
#include "core/application.h"
#include "covermanager/covermanagermenu.h"
#include "core/settings.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "dialogs/dialoghelpers.h"
#include "dialogs/dialoglistkeyboard.h"
#include "dialogs/edittagcompleter.h"
#include "dialogs/edittagcover.h"
#include "dialogs/edittagcoverdrop.h"
#include "dialogs/edittagfieldreset.h"
#include "dialogs/edittagfields.h"
#include "dialogs/edittagid3v2.h"
#include "dialogs/edittagloading.h"
#include "dialogs/edittagsummaryfields.h"
#include "tagreader/tagreaderclient.h"
#include "dialogs/edittagsummarylabels.h"
#include "dialogs/edittagtabs.h"
#include "tagreader/savetagsoptions.h"
#include "tagreader/tagid3v2version.h"
#include "dialogs/trackselectiondialog.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/timeutils.h"
#include "widgets/listboxkeyboardgtk.h"

#include <adwaita.h>
#include <memory>
#include <thread>
#include <utility>

using DialogHelpers::PrettyBytes;
using DialogHelpers::PrettyUnixTime;
using DialogHelpers::SetImageFromBytes;
using DialogHelpers::SongForDialog;

namespace {

struct State {
  Application *app = nullptr;
  GtkWindow *parent = nullptr;
  Song song;
  SongList songs;
  size_t index = 0;
  GtkWidget *cover = nullptr;
  GtkWidget *tags_art = nullptr;
  GtkWidget *tags_art_button = nullptr;
  GtkWidget *lyrics = nullptr;
  GtkWidget *lyrics_label = nullptr;
  GtkWidget *lyrics_reset = nullptr;
  GtkWidget *rating = nullptr;
  GtkWidget *rating_label = nullptr;
  GtkWidget *rating_reset = nullptr;
  double initial_rating = 0;
  double initial_rating_stored = -1;
  GtkWidget *compilation = nullptr;
  GtkWidget *stats_label = nullptr;
  GtkWidget *summary_title = nullptr;
  GtkWidget *summary_grid = nullptr;
  GtkWidget *tags_summary = nullptr;
  GtkWidget *stats_plays = nullptr;
  GtkWidget *stats_skips = nullptr;
  GtkWidget *stats_last = nullptr;
  GtkWidget *stats_path = nullptr;
  GtkWidget *song_list = nullptr;
  GtkWidget *prev = nullptr;
  GtkWidget *next = nullptr;
  GtkWidget *id3v2 = nullptr;
  GtkWidget *embedded_cover = nullptr;
  GtkWidget *stack = nullptr;
  GtkWidget *actions = nullptr;
  GtkWidget *fetch_tags = nullptr;
  GtkWidget *loading_label = nullptr;
  GtkWidget *fetch_cover = nullptr;
  GtkWidget *search_cover = nullptr;
  GtkWidget *url_cover = nullptr;
  GtkWidget *file_cover = nullptr;
  GtkWidget *unset_cover = nullptr;
  GtkWidget *clear_cover = nullptr;
  GtkWidget *delete_cover = nullptr;
  GtkWidget *show_cover = nullptr;
  GtkWidget *save_cover = nullptr;
  GtkWidget *cover_hint = nullptr;
  GtkWidget *summary_page = nullptr;
  GtkWidget *lyrics_page = nullptr;
  GtkWidget *fetch_lyrics = nullptr;
  bool loading = false;
#ifdef HAVE_TAGFETCHER
  bool have_tagfetcher = true;
#else
  bool have_tagfetcher = false;
#endif
  std::unique_ptr<AlbumCoverChoiceController> covers;
  std::vector<std::pair<std::string, GtkWidget *>> fields;
  std::vector<std::string> initial;
  AdwDialog *dialog = nullptr;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);

  ~State() {
    if (alive) {
      *alive = false;
    }
  }
};

struct EditTagLoadIdle {
  State *state = nullptr;
  SongList songs;
  std::shared_ptr<bool> alive;
};

void UpdateDisplay(State *state);

std::string TextOf(GtkWidget *widget) {
  if (!widget) {
    return {};
  }
  if (GTK_IS_TEXT_VIEW(widget)) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    std::string result = text ? text : "";
    g_free(text);
    return result;
  }
  return gtk_editable_get_text(GTK_EDITABLE(widget));
}

void SetText(GtkWidget *widget, const std::string &value) {
  if (!widget) {
    return;
  }
  if (GTK_IS_TEXT_VIEW(widget)) {
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), value.c_str(), -1);
    return;
  }
  gtk_editable_set_text(GTK_EDITABLE(widget), value.c_str());
}

GtkWidget *MakeFieldResetButton() {
  GtkWidget *reset = gtk_button_new_from_icon_name("edit-undo-symbolic");
  gtk_widget_add_css_class(reset, "flat");
  gtk_widget_set_tooltip_text(reset, Translations::CStr(EditTagFieldReset::ResetTooltip()));
  gtk_widget_set_valign(reset, GTK_ALIGN_CENTER);
  return reset;
}

void SetModifiedStyle(GtkWidget *widget, bool modified) {
  if (!widget) {
    return;
  }
  if (modified) {
    gtk_widget_add_css_class(widget, "heading");
  } else {
    gtk_widget_remove_css_class(widget, "heading");
  }
}

void AttachEntryReset(AdwEntryRow *entry, const std::string &name, const std::string &initial) {
  GtkWidget *reset = MakeFieldResetButton();
  adw_entry_row_add_suffix(entry, reset);
  g_object_set_data(G_OBJECT(entry), "reset-btn", reset);
  g_object_set_data_full(G_OBJECT(entry), "initial-text", g_strdup(initial.c_str()), g_free);
  g_object_set_data_full(G_OBJECT(entry), "field-name", g_strdup(name.c_str()), g_free);
  g_object_set_data(G_OBJECT(reset), "entry", entry);
  gtk_widget_set_visible(reset, FALSE);
  g_signal_connect(entry, "changed", G_CALLBACK((+[](AdwEntryRow *row, gpointer) {
                     auto *btn = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "reset-btn"));
                     const char *initial_text = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "initial-text"));
                     const char *field_name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "field-name"));
                     const bool modified = EditTagFields::IsValueModified(field_name ? field_name : "", initial_text ? initial_text : "",
                                                                          gtk_editable_get_text(GTK_EDITABLE(row)));
                     SetModifiedStyle(GTK_WIDGET(row), modified);
                     if (btn) {
                       gtk_widget_set_visible(btn, modified);
                     }
                   })),
                   nullptr);
  g_signal_connect(reset, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer) {
                     auto *field = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "entry"));
                     const char *initial_text = field ? static_cast<const char *>(g_object_get_data(G_OBJECT(field), "initial-text")) : nullptr;
                     if (field) {
                       gtk_editable_set_text(GTK_EDITABLE(field), initial_text ? initial_text : "");
                     }
                   }),
                   nullptr);
}

void AttachFieldCompletion(AdwEntryRow *entry, const std::string &name, const SongList &library) {
  if (!entry || !EditTagCompleter::CompletesField(name)) {
    return;
  }
  auto *values = new std::vector<std::string>(EditTagCompleter::ValuesFor(library, name));
  GtkWidget *popover = gtk_popover_new();
  GtkWidget *list = gtk_list_box_new();
  gtk_popover_set_child(GTK_POPOVER(popover), list);
  gtk_widget_set_parent(popover, GTK_WIDGET(entry));
  g_object_set_data_full(G_OBJECT(entry), "complete-values", values, [](gpointer p) { delete static_cast<std::vector<std::string> *>(p); });
  g_object_set_data(G_OBJECT(entry), "complete-popover", popover);
  g_object_set_data(G_OBJECT(entry), "complete-list", list);
  g_signal_connect(entry, "changed", G_CALLBACK(+[](AdwEntryRow *row, gpointer) {
                     auto *values = static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(row), "complete-values"));
                     auto *popover = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "complete-popover"));
                     auto *list = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "complete-list"));
                     if (!values || !popover || !list) {
                       return;
                     }
                     gtk_list_box_remove_all(GTK_LIST_BOX(list));
                     const std::vector<std::string> matches =
                         EditTagCompleter::Suggestions(*values, gtk_editable_get_text(GTK_EDITABLE(row)));
                     for (const std::string &match : matches) {
                       GtkWidget *label = gtk_label_new(match.c_str());
                       gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
                       gtk_list_box_append(GTK_LIST_BOX(list), label);
                     }
                     if (matches.empty()) {
                       gtk_popover_popdown(GTK_POPOVER(popover));
                     } else {
                       gtk_popover_popup(GTK_POPOVER(popover));
                     }
                   }),
                   nullptr);
  g_signal_connect(list, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     GtkWidget *label = gtk_list_box_row_get_child(row);
                     auto *entry = GTK_WIDGET(data);
                     if (label && GTK_IS_LABEL(label) && entry) {
                       gtk_editable_set_text(GTK_EDITABLE(entry), gtk_label_get_text(GTK_LABEL(label)));
                     }
                     if (auto *popover = GTK_WIDGET(g_object_get_data(G_OBJECT(entry), "complete-popover"))) {
                       gtk_popover_popdown(GTK_POPOVER(popover));
                     }
                   }),
                   entry);
  g_signal_connect(entry, "destroy", G_CALLBACK(+[](GtkWidget *widget, gpointer) {
                     if (GtkWidget *popover = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "complete-popover"))) {
                       gtk_widget_unparent(popover);
                     }
                   }),
                   nullptr);
}

void FillSummaryGrid(State *state) {
  if (!state) {
    return;
  }
  if (state->summary_title) {
    gtk_label_set_text(GTK_LABEL(state->summary_title), state->song.PrettyTitleWithArtist().c_str());
  }
  if (!state->summary_grid) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(state->summary_grid);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_grid_remove(GTK_GRID(state->summary_grid), child);
    child = next;
  }
  int row = 0;
  for (const EditTagSummaryFields::Row &info : EditTagSummaryFields::Rows(state->song)) {
    if (info.value.empty()) {
      continue;
    }
    GtkWidget *key = gtk_label_new(Translations::CStr(info.label));
    gtk_widget_add_css_class(key, "dim-label");
    gtk_label_set_xalign(GTK_LABEL(key), 0.0f);
    GtkWidget *value = gtk_label_new(info.value.c_str());
    gtk_label_set_xalign(GTK_LABEL(value), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(value), TRUE);
    gtk_label_set_selectable(GTK_LABEL(value), TRUE);
    gtk_widget_set_hexpand(value, TRUE);
    gtk_grid_attach(GTK_GRID(state->summary_grid), key, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(state->summary_grid), value, 1, row, 1, 1);
    ++row;
  }
}

std::string TechnicalLine(const Song &song) {
  std::string line = PrettyBytes(song.filesize()) + " · " + Utilities::PrettyTimeNanosec(song.length_nanosec());
  if (song.bitrate() > 0) {
    line += " · " + std::to_string(song.bitrate()) + " kbps";
  }
  if (song.samplerate() > 0) {
    line += " · " + std::to_string(song.samplerate()) + " Hz";
  }
  if (song.bitdepth() > 0) {
    line += " · " + std::to_string(song.bitdepth()) + "-bit";
  }
  line += " · " + Song::FiletypeToString(song.filetype());
  return line;
}

void ApplyCoverEnable(State *state) {
  if (!state) {
    return;
  }
  const bool change_art = EditTagCover::ChangeArtEnabled(state->song);
  bool has_providers = false;
  if (state->app && state->app->cover_providers()) {
    has_providers = CoverManagerMenu::HasAnyProviders(state->app->cover_providers()->All().size());
  }
  const bool art_different = EditTagCover::ArtDifferentAcrossSongs(state->songs);
  if (state->cover_hint) {
    gtk_widget_set_visible(state->cover_hint, art_different);
  }
  if (state->fetch_cover) {
    gtk_widget_set_sensitive(state->fetch_cover, EditTagCover::FetchCoverEnabled(change_art));
  }
  if (state->search_cover) {
    gtk_widget_set_sensitive(state->search_cover, EditTagCover::SearchCoverEnabled(has_providers, change_art, art_different));
  }
  if (state->url_cover) {
    gtk_widget_set_sensitive(state->url_cover, EditTagCover::FromUrlEnabled(change_art));
  }
  if (state->file_cover) {
    gtk_widget_set_sensitive(state->file_cover, EditTagCover::FromFileEnabled(change_art));
  }
  if (state->unset_cover) {
    gtk_widget_set_sensitive(state->unset_cover, EditTagCover::UnsetCoverEnabled(state->song, change_art, art_different));
  }
  if (state->clear_cover) {
    gtk_widget_set_sensitive(state->clear_cover, EditTagCover::ClearCoverEnabled(state->song, change_art, art_different));
  }
  if (state->delete_cover) {
    gtk_widget_set_sensitive(state->delete_cover, EditTagCover::DeleteCoverEnabled(state->song, change_art, art_different));
  }
  if (state->show_cover) {
    gtk_widget_set_sensitive(state->show_cover, EditTagCover::ShowCoverEnabled(state->song, art_different));
  }
  if (state->tags_art_button) {
    gtk_widget_set_sensitive(state->tags_art_button, EditTagCover::ChangeArtEnabled(state->song) ? TRUE : FALSE);
  }
  if (state->save_cover) {
    gtk_widget_set_sensitive(state->save_cover, EditTagCover::SaveCoverEnabled(state->song, art_different));
  }
}

void ShowCoverIfAllowed(State *state) {
  if (!state || !state->covers) {
    return;
  }
  const bool art_different = EditTagCover::ArtDifferentAcrossSongs(state->songs);
  if (!EditTagCover::ShowOnDoubleClick(state->song, art_different)) {
    return;
  }
  state->covers->ShowCover(state->parent, state->song);
}

void AttachCoverGesture(GtkWidget *widget, State *state, bool menu_on_click) {
  if (!widget || !state) {
    return;
  }
  GtkGesture *click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
  g_object_set_data(G_OBJECT(click), "menu-on-click", GINT_TO_POINTER(menu_on_click ? 2 : 1));
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(click));
  g_signal_connect(click, "released", G_CALLBACK((+[](GtkGestureClick *gesture, gint n_press, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     const guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
                     if (EditTagCover::IsDoubleClick(n_press, button)) {
                       ShowCoverIfAllowed(self);
                       return;
                     }
                     const bool menu_on_click = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(gesture), "menu-on-click")) == 2;
                     if (menu_on_click && EditTagCover::IsMenuClick(n_press, button) && self && self->tags_art_button &&
                         GTK_IS_MENU_BUTTON(self->tags_art_button)) {
                       gtk_menu_button_popup(GTK_MENU_BUTTON(self->tags_art_button));
                     }
                   })),
                   state);
}

void AttachCoverDrop(GtkWidget *widget, State *state) {
  if (!widget || !state) {
    return;
  }
  GtkDropTarget *drop = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(drop));
  g_signal_connect(drop, "drop", G_CALLBACK((+[](GtkDropTarget *, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     auto *self = static_cast<State *>(data);
                     if (!self || !self->app || !G_VALUE_HOLDS_STRING(value)) {
                       return FALSE;
                     }
                     const char *text = g_value_get_string(value);
                     const std::string path = EditTagCoverDrop::FirstImagePath(text ? text : "");
                     if (path.empty() || !self->covers || !self->covers->SaveCover(&self->song, path)) {
                       return FALSE;
                     }
                     if (self->index < self->songs.size()) {
                       self->songs[self->index] = self->song;
                     }
                     UpdateDisplay(self);
                     return TRUE;
                   })),
                   state);
}

void ApplyEditEnable(State *state) {
  if (!state) {
    return;
  }
  const bool has_valid = !state->songs.empty();
  const bool list_visible = EditTagFields::SongListVisible(state->songs.size());
  if (state->actions) {
    gtk_widget_set_sensitive(state->actions, EditTagFields::ButtonsEnabled(state->loading));
  }
  if (state->stack) {
    gtk_widget_set_sensitive(state->stack, EditTagFields::FieldsEnabled(state->loading, has_valid));
  }
  if (state->song_list) {
    gtk_widget_set_visible(state->song_list, list_visible);
    gtk_widget_set_sensitive(state->song_list, EditTagFields::SongListEnabled(state->loading, has_valid));
  }
  if (state->prev) {
    gtk_widget_set_sensitive(state->prev, EditTagFields::SongListNavEnabled(list_visible, state->loading));
  }
  if (state->next) {
    gtk_widget_set_sensitive(state->next, EditTagFields::SongListNavEnabled(list_visible, state->loading));
  }
  if (state->fetch_tags) {
    gtk_widget_set_sensitive(state->fetch_tags, EditTagFields::FetchTagsEnabled(state->have_tagfetcher, state->loading));
  }
  if (state->loading_label) {
    gtk_widget_set_visible(state->loading_label, EditTagFields::LoadingLabelVisible(state->loading));
  }
  const bool fields_on = EditTagFields::FieldsEnabled(state->loading, has_valid);
  for (const auto &field : state->fields) {
    if (field.second) {
      gtk_widget_set_sensitive(field.second, fields_on && EditTagFields::FieldEnabled(field.first, state->songs));
    }
  }
  if (state->compilation) {
    gtk_widget_set_sensitive(state->compilation, fields_on && EditTagFields::FieldEnabled("Compilation", state->songs));
  }
  if (state->rating) {
    gtk_widget_set_sensitive(state->rating, fields_on && EditTagFields::FieldEnabled("Rating", state->songs));
  }
  const bool lyrics_on = fields_on && EditTagCover::LyricsTabEnabled(state->songs.size());
  if (state->lyrics_page) {
    gtk_widget_set_sensitive(state->lyrics_page, lyrics_on);
  }
  if (state->fetch_lyrics) {
    gtk_widget_set_sensitive(state->fetch_lyrics, lyrics_on);
  }
  const bool summary_on = fields_on && EditTagCover::SummaryTabEnabled(state->songs.size());
  if (state->summary_page) {
    gtk_widget_set_sensitive(state->summary_page, summary_on);
  }
  if (state->stack && ADW_IS_VIEW_STACK(state->stack)) {
    const char *name = adw_view_stack_get_visible_child_name(ADW_VIEW_STACK(state->stack));
    const int visible = EditTagTabs::VisibleIndex(EditTagTabs::IndexFromName(name ? name : ""), state->songs.size());
    if (g_strcmp0(name, EditTagTabs::Name(visible)) != 0) {
      adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(state->stack), EditTagTabs::Name(visible));
    }
  }
  ApplyCoverEnable(state);
}

void UpdateDisplay(State *state) {
  if (!state) {
    return;
  }
  if (state->index >= state->songs.size()) {
    ApplyEditEnable(state);
    return;
  }
  state->song = state->songs[state->index];
  if (state->app) {
    const std::vector<unsigned char> art = state->app->albumcover_loader()->LoadData(state->song);
    if (state->cover) {
      SetImageFromBytes(state->cover, art, EditTagCover::kSummaryArtSize);
    }
    if (state->tags_art) {
      SetImageFromBytes(state->tags_art, art, EditTagCover::kTagsArtSize);
    }
  }
  FillSummaryGrid(state);
  if (state->stats_label) {
    const std::string stats = "Plays: " + std::to_string(state->song.playcount()) + "   Skips: " + std::to_string(state->song.skipcount()) +
                              "   Last played: " + PrettyUnixTime(state->song.lastplayed()) + "\n" + FileUtils::PathFromUri(state->song.url()) +
                              "\n" + TechnicalLine(state->song);
    gtk_label_set_text(GTK_LABEL(state->stats_label), stats.c_str());
  }
  if (state->stats_plays) {
    gtk_label_set_text(GTK_LABEL(state->stats_plays), std::to_string(state->song.playcount()).c_str());
  }
  if (state->stats_skips) {
    gtk_label_set_text(GTK_LABEL(state->stats_skips), std::to_string(state->song.skipcount()).c_str());
  }
  if (state->stats_last) {
    gtk_label_set_text(GTK_LABEL(state->stats_last), PrettyUnixTime(state->song.lastplayed()).c_str());
  }
  if (state->stats_path) {
    gtk_label_set_text(GTK_LABEL(state->stats_path),
                       (FileUtils::PathFromUri(state->song.url()) + "\n" + TechnicalLine(state->song)).c_str());
  }
  if (state->song_list) {
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->song_list), static_cast<int>(state->index));
    if (row) {
      gtk_list_box_select_row(GTK_LIST_BOX(state->song_list), row);
    }
  }
  if (state->embedded_cover && state->covers) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->embedded_cover),
                                EditTagCover::DefaultEmbeddedChecked(state->song, state->covers->get_collection_save_album_cover_type()));
    gtk_widget_set_sensitive(state->embedded_cover, state->song.save_embedded_cover_supported());
  }
  ApplyEditEnable(state);
}

void SelectSong(State *state, int index) {
  if (!state || state->songs.empty()) {
    return;
  }
  state->index = static_cast<size_t>(EditTagFields::WrapIndex(index, 0, static_cast<int>(state->songs.size())));
  UpdateDisplay(state);
}

void SetFieldValue(State *state, const std::string &name, const std::pair<std::string, bool> &value) {
  if (!state) {
    return;
  }
  for (size_t i = 0; i < state->fields.size(); ++i) {
    if (state->fields[i].first != name || !state->fields[i].second) {
      continue;
    }
    SetText(state->fields[i].second, value.first);
    if (i < state->initial.size()) {
      state->initial[i] = value.first;
    }
    g_object_set_data_full(G_OBJECT(state->fields[i].second), "initial-text", g_strdup(value.first.c_str()), g_free);
    if (value.second) {
      gtk_widget_set_tooltip_text(state->fields[i].second, Translations::CStr("Multiple values — type to set all selected songs"));
    } else {
      gtk_widget_set_tooltip_text(state->fields[i].second, nullptr);
    }
  }
}

void RefreshTagFields(State *state) {
  if (!state) {
    return;
  }
  const SongList &songs = state->songs;
  SetFieldValue(state, "Title", EditTagFields::CommonValue(songs, [](const Song &s) { return s.title(); }));
  SetFieldValue(state, "Artist", EditTagFields::CommonValue(songs, [](const Song &s) { return s.artist(); }));
  SetFieldValue(state, "Album", EditTagFields::CommonValue(songs, [](const Song &s) { return s.album(); }));
  SetFieldValue(state, "Album artist", EditTagFields::CommonValue(songs, [](const Song &s) { return s.albumartist(); }));
  SetFieldValue(state, "Year",
                EditTagFields::CommonValue(songs, [](const Song &s) { return s.year() > 0 ? std::to_string(s.year()) : std::string(); }));
  SetFieldValue(state, "Original year",
                EditTagFields::CommonValue(songs, [](const Song &s) { return s.originalyear() > 0 ? std::to_string(s.originalyear()) : std::string(); }));
  SetFieldValue(state, "Track",
                EditTagFields::CommonValue(songs, [](const Song &s) { return s.track() > 0 ? std::to_string(s.track()) : std::string(); }));
  SetFieldValue(state, "Genre", EditTagFields::CommonValue(songs, [](const Song &s) { return s.genre(); }));
  SetFieldValue(state, "Composer", EditTagFields::CommonValue(songs, [](const Song &s) { return s.composer(); }));
  SetFieldValue(state, "Performer", EditTagFields::CommonValue(songs, [](const Song &s) { return s.performer(); }));
  SetFieldValue(state, "Grouping", EditTagFields::CommonValue(songs, [](const Song &s) { return s.grouping(); }));
  SetFieldValue(state, "Comment", EditTagFields::CommonValue(songs, [](const Song &s) { return s.comment(); }));
  SetFieldValue(state, "Disc",
                EditTagFields::CommonValue(songs, [](const Song &s) { return s.disc() > 0 ? std::to_string(s.disc()) : std::string(); }));
  SetFieldValue(state, "BPM",
                EditTagFields::CommonValue(songs, [](const Song &s) { return s.bpm() > 0 ? std::to_string(s.bpm()) : std::string(); }));
  SetFieldValue(state, "Mood", EditTagFields::CommonValue(songs, [](const Song &s) { return s.mood(); }));
  SetFieldValue(state, "Initial key", EditTagFields::CommonValue(songs, [](const Song &s) { return s.initial_key(); }));
  SetFieldValue(state, "Title sort", EditTagFields::CommonValue(songs, [](const Song &s) { return s.titlesort(); }));
  SetFieldValue(state, "Artist sort", EditTagFields::CommonValue(songs, [](const Song &s) { return s.artistsort(); }));
  SetFieldValue(state, "Album sort", EditTagFields::CommonValue(songs, [](const Song &s) { return s.albumsort(); }));
  SetFieldValue(state, "Album artist sort", EditTagFields::CommonValue(songs, [](const Song &s) { return s.albumartistsort(); }));
  SetFieldValue(state, "Composer sort", EditTagFields::CommonValue(songs, [](const Song &s) { return s.composersort(); }));
  SetFieldValue(state, "Performer sort", EditTagFields::CommonValue(songs, [](const Song &s) { return s.performersort(); }));
  SetFieldValue(state, "Lyrics", EditTagFields::CommonValue(songs, [](const Song &s) { return s.lyrics(); }));
  if (state->rating) {
    state->initial_rating_stored = EditTagFields::CommonRating(songs);
    state->initial_rating = EditTagFields::RatingSliderFromStored(state->initial_rating_stored);
    gtk_range_set_value(GTK_RANGE(state->rating), state->initial_rating);
  }
  if (state->compilation && !songs.empty()) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->compilation), songs.front().compilation());
  }
  if (state->tags_summary) {
    gtk_label_set_text(GTK_LABEL(state->tags_summary), EditTagCompleter::TagsSummary(static_cast<int>(songs.size())).c_str());
  }
}

void RebuildSongList(State *state) {
  if (!state || !state->song_list) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(state->song_list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(state->song_list), child);
    child = next;
  }
  for (size_t i = 0; i < state->songs.size(); ++i) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), EditTagFields::SongRowLabel(state->songs[i]).c_str());
    g_object_set_data(G_OBJECT(row), "song-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    gtk_list_box_append(GTK_LIST_BOX(state->song_list), GTK_WIDGET(row));
  }
}

void SetLoading(State *state, bool loading) {
  if (!state) {
    return;
  }
  state->loading = loading;
  if (state->loading_label) {
    gtk_label_set_text(GTK_LABEL(state->loading_label), Translations::CStr(EditTagFields::LoadingTracksMessage()));
  }
  ApplyEditEnable(state);
}

void ApplyLoadedSongs(State *state, SongList songs) {
  if (!state) {
    return;
  }
  state->songs = std::move(songs);
  state->index = 0;
  if (!state->songs.empty()) {
    state->song = state->songs.front();
  } else {
    state->song = Song();
  }
  if (state->dialog) {
    const std::string dialog_title = state->songs.size() > 1
                                         ? Translations::Tr("Edit tags") + " (" + std::to_string(state->songs.size()) + " " + Translations::Tr("songs") + ")"
                                         : Translations::Tr("Edit tags");
    adw_dialog_set_title(state->dialog, dialog_title.c_str());
  }
  RebuildSongList(state);
  RefreshTagFields(state);
  SetLoading(state, false);
  UpdateDisplay(state);
}

void StartTagLoad(State *state, const SongList &incoming) {
  if (!state) {
    return;
  }
  TagReaderClient *client = state->app ? state->app->tagreader_client() : nullptr;
  if (!client) {
    SetLoading(state, false);
    return;
  }
  SetLoading(state, true);
  const std::shared_ptr<bool> alive = state->alive;
  std::thread([state, client, incoming, alive]() {
    const SongList songs = EditTagLoading::LoadData(incoming, [client](const std::string &path, Song *song) {
      return client->ReadFileBlocking(path, song);
    });
    auto *idle = new EditTagLoadIdle{state, songs, alive};
    g_idle_add(+[](gpointer data) -> gboolean {
      auto *loaded = static_cast<EditTagLoadIdle *>(data);
      if (loaded->alive && *loaded->alive && loaded->state) {
        ApplyLoadedSongs(loaded->state, loaded->songs);
      }
      delete loaded;
      return G_SOURCE_REMOVE;
    }, idle);
  }).detach();
}

void PersistPlayStatistics(State *state, Song *song) {
  if (!state || !state->app || !song) {
    return;
  }
  if (song->id() > 0) {
    state->app->collection()->backend()->ResetPlayStatistics(song->id());
  }
  const std::string path = FileUtils::PathFromUri(song->url());
  if (!path.empty()) {
    state->app->tagreader()->SavePlaycount(path, 0);
  }
}

}  // namespace

void EditTagDialog::Show(GtkWindow *parent, Application *app, const SongList &songs) {
  SongList incoming = songs;
  if (incoming.empty()) {
    incoming.push_back(SongForDialog(app));
  }
  const SongList targets = EditTagFields::ValidSongs(incoming);
  auto *state = new State();
  state->app = app;
  state->parent = parent;
  state->songs = targets;
  state->index = 0;
  if (!targets.empty()) {
    state->song = targets.front();
  }
  state->covers = std::make_unique<AlbumCoverChoiceController>(app);

  AdwDialog *dialog = adw_dialog_new();
  const std::string dialog_title = targets.size() > 1
                                      ? Translations::Tr("Edit tags") + " (" + std::to_string(targets.size()) + " " + Translations::Tr("songs") + ")"
                                      : Translations::Tr("Edit tags");
  adw_dialog_set_title(dialog, dialog_title.c_str());
  adw_dialog_set_content_width(dialog, 640);
  adw_dialog_set_content_height(dialog, 760);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  g_object_set_data_full(G_OBJECT(dialog), "state", state, [](gpointer p) { delete static_cast<State *>(p); });

  state->loading_label = gtk_label_new(Translations::CStr(EditTagFields::LoadingTracksMessage()));
  gtk_widget_add_css_class(state->loading_label, "dim-label");
  gtk_widget_set_margin_start(state->loading_label, 12);
  gtk_widget_set_margin_end(state->loading_label, 12);
  gtk_widget_set_margin_top(state->loading_label, 8);
  gtk_widget_set_visible(state->loading_label, EditTagFields::LoadingLabelVisible(state->loading));
  gtk_box_append(GTK_BOX(box), state->loading_label);

  if (EditTagFields::SongListVisible(targets.size())) {
    GtkWidget *nav = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(nav, 12);
    gtk_widget_set_margin_end(nav, 12);
    gtk_widget_set_margin_top(nav, 8);
    state->prev = gtk_button_new_with_label(Translations::CStr("Previous"));
    state->next = gtk_button_new_with_label(Translations::CStr("Next"));
    gtk_box_append(GTK_BOX(nav), state->prev);
    gtk_box_append(GTK_BOX(nav), state->next);
    gtk_box_append(GTK_BOX(box), nav);
    state->song_list = gtk_list_box_new();
    gtk_widget_add_css_class(state->song_list, "boxed-list");
    gtk_widget_set_margin_start(state->song_list, 12);
    gtk_widget_set_margin_end(state->song_list, 12);
    for (size_t i = 0; i < targets.size(); ++i) {
      AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), EditTagFields::SongRowLabel(targets[i]).c_str());
      g_object_set_data(G_OBJECT(row), "song-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
      gtk_list_box_append(GTK_LIST_BOX(state->song_list), GTK_WIDGET(row));
    }
    gtk_box_append(GTK_BOX(box), state->song_list);
    g_signal_connect(state->prev, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *self = static_cast<State *>(data);
                       SelectSong(self, EditTagFields::WrapIndex(static_cast<int>(self->index), -1, static_cast<int>(self->songs.size())));
                     })),
                     state);
    g_signal_connect(state->next, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *self = static_cast<State *>(data);
                       SelectSong(self, EditTagFields::WrapIndex(static_cast<int>(self->index), 1, static_cast<int>(self->songs.size())));
                     })),
                     state);
    g_signal_connect(state->song_list, "row-activated", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "song-index")) - 1;
                       SelectSong(static_cast<State *>(data), index);
                     })),
                     state);
    gtk_widget_set_focusable(state->song_list, TRUE);
    GtkEventController *keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
    gtk_widget_add_controller(state->song_list, keys);
    g_signal_connect(keys, "key-pressed",
                     G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                       auto *self = static_cast<State *>(data);
                       if (DialogListKeyboard::IsActivate(keyval)) {
                         ListBoxKeyboardGtk::ActivateSelected(self->song_list);
                         return TRUE;
                       }
                       if (DialogListKeyboard::IsMove(keyval)) {
                         SelectSong(self, DialogListKeyboard::NextIndex(static_cast<int>(self->index), static_cast<int>(self->songs.size()), keyval));
                         return TRUE;
                       }
                       return FALSE;
                     })),
                     state);
  }

  AdwViewStack *stack = ADW_VIEW_STACK(adw_view_stack_new());
  state->stack = GTK_WIDGET(stack);
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), stack);

  SongList library = targets;
  if (app && app->collection()) {
    const SongList collection = app->collection()->Songs();
    if (!collection.empty()) {
      library = collection;
    }
  }

  auto add_entries = [&](GtkWidget *page, const std::vector<std::pair<const char *, std::pair<std::string, bool>>> &rows) {
    for (const auto &row : rows) {
      AdwEntryRow *entry = ADW_ENTRY_ROW(adw_entry_row_new());
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), Translations::CStr(row.first));
      gtk_editable_set_text(GTK_EDITABLE(entry), row.second.first.c_str());
      if (row.second.second) {
        gtk_widget_set_tooltip_text(GTK_WIDGET(entry), Translations::CStr("Multiple values — type to set all selected songs"));
      }
      state->fields.emplace_back(row.first, GTK_WIDGET(entry));
      state->initial.push_back(row.second.first);
      AttachEntryReset(entry, row.first, row.second.first);
      AttachFieldCompletion(entry, row.first, library);
      gtk_box_append(GTK_BOX(page), GTK_WIDGET(entry));
    }
  };

  GtkWidget *summary = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(summary, 12);
  gtk_widget_set_margin_end(summary, 12);
  gtk_widget_set_margin_top(summary, 12);
  state->summary_page = summary;
  state->cover = gtk_image_new();
  gtk_widget_set_halign(state->cover, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(summary), state->cover);
  state->cover_hint = gtk_label_new(Translations::CStr(EditTagCover::ArtDifferentHint()));
  gtk_widget_add_css_class(state->cover_hint, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(state->cover_hint), TRUE);
  gtk_widget_set_halign(state->cover_hint, GTK_ALIGN_CENTER);
  gtk_widget_set_visible(state->cover_hint, FALSE);
  gtk_box_append(GTK_BOX(summary), state->cover_hint);
  AttachCoverGesture(state->cover, state, false);
  AttachCoverDrop(state->cover, state);
  GtkWidget *cover_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign(cover_buttons, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(cover_buttons, TRUE);
  GtkWidget *cover_buttons2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign(cover_buttons2, GTK_ALIGN_CENTER);
  state->fetch_cover = gtk_button_new_with_label(Translations::CStr("Fetch cover"));
  state->search_cover = gtk_button_new_with_label(Translations::CStr("Search…"));
  state->url_cover = gtk_button_new_with_label(Translations::CStr("From URL"));
  state->file_cover = gtk_button_new_with_label(Translations::CStr("From file"));
  state->unset_cover = gtk_button_new_with_label(Translations::CStr("Unset"));
  state->clear_cover = gtk_button_new_with_label(Translations::CStr("Clear"));
  state->delete_cover = gtk_button_new_with_label(Translations::CStr("Delete"));
  state->show_cover = gtk_button_new_with_label(Translations::CStr("Show"));
  state->save_cover = gtk_button_new_with_label(Translations::CStr("Save…"));
  GtkWidget *stats_cover = gtk_button_new_with_label(Translations::CStr("Statistics"));
  gtk_box_append(GTK_BOX(cover_buttons), state->fetch_cover);
  gtk_box_append(GTK_BOX(cover_buttons), state->search_cover);
  gtk_box_append(GTK_BOX(cover_buttons), state->url_cover);
  gtk_box_append(GTK_BOX(cover_buttons), state->file_cover);
  gtk_box_append(GTK_BOX(cover_buttons), state->unset_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), state->clear_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), state->delete_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), state->show_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), state->save_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), stats_cover);
  gtk_box_append(GTK_BOX(summary), cover_buttons);
  gtk_box_append(GTK_BOX(summary), cover_buttons2);
  state->summary_title = gtk_label_new("");
  gtk_widget_add_css_class(state->summary_title, "title-3");
  gtk_label_set_wrap(GTK_LABEL(state->summary_title), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->summary_title), 0.0f);
  gtk_box_append(GTK_BOX(summary), state->summary_title);
  state->summary_grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(state->summary_grid), 12);
  gtk_grid_set_row_spacing(GTK_GRID(state->summary_grid), 4);
  gtk_box_append(GTK_BOX(summary), state->summary_grid);
  state->stats_label = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(state->stats_label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->stats_label), 0);
  gtk_widget_add_css_class(state->stats_label, "dim-label");
  gtk_widget_set_visible(state->stats_label, FALSE);
  adw_view_stack_add_titled(stack, summary, "Summary", Translations::CStr("Summary"));

  GtkWidget *tags = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(tags, 12);
  gtk_widget_set_margin_end(tags, 12);
  gtk_widget_set_margin_top(tags, 12);
  GtkWidget *tags_art_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  GtkWidget *tags_art_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  state->tags_art = gtk_image_new();
  gtk_widget_set_size_request(state->tags_art, EditTagCover::kTagsArtSize, EditTagCover::kTagsArtSize);
  gtk_widget_set_halign(state->tags_art, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(tags_art_col), state->tags_art);
  GtkWidget *change_art_popover = gtk_popover_new();
  GtkWidget *change_art_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_popover_set_child(GTK_POPOVER(change_art_popover), change_art_box);
  state->tags_art_button = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(state->tags_art_button), Translations::CStr(EditTagCover::ChangeArt()));
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(state->tags_art_button), change_art_popover);
  g_object_set_data(G_OBJECT(state->tags_art_button), "menu-box", change_art_box);
  gtk_box_append(GTK_BOX(tags_art_col), state->tags_art_button);
  state->embedded_cover = gtk_check_button_new_with_label(Translations::CStr("Embedded cover"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->embedded_cover),
                              EditTagCover::DefaultEmbeddedChecked(state->song, state->covers->get_collection_save_album_cover_type()));
  gtk_widget_set_sensitive(state->embedded_cover, EditTagCover::AnySupported(targets));
  gtk_box_append(GTK_BOX(tags_art_col), state->embedded_cover);
  g_signal_connect(state->embedded_cover, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self->covers) {
                       self->covers->set_save_embedded_cover_override(gtk_check_button_get_active(button));
                     }
                   }),
                   state);
  if (EditTagId3v2::AnySupported(targets)) {
    GtkWidget *id3_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *id3_label = gtk_label_new(Translations::CStr("ID3v2 version:"));
    gtk_label_set_xalign(GTK_LABEL(id3_label), 0);
    GtkStringList *versions = gtk_string_list_new(nullptr);
    gtk_string_list_append(versions, "2.3");
    gtk_string_list_append(versions, "2.4");
    state->id3v2 = gtk_drop_down_new(G_LIST_MODEL(versions), nullptr);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(state->id3v2), EditTagId3v2::ComboIndex(EditTagId3v2::VersionForSongs(targets)));
    gtk_widget_set_hexpand(state->id3v2, TRUE);
    gtk_box_append(GTK_BOX(id3_row), id3_label);
    gtk_box_append(GTK_BOX(id3_row), state->id3v2);
    gtk_box_append(GTK_BOX(tags_art_col), id3_row);
  }
  GtkWidget *fetch_tags_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *fetch_tags_icon = gtk_image_new_from_resource(EditTagSummaryLabels::MusicBrainzIconResource());
  gtk_widget_set_size_request(fetch_tags_icon, EditTagSummaryLabels::kMusicBrainzIconWidth, EditTagSummaryLabels::kMusicBrainzIconHeight);
  gtk_box_append(GTK_BOX(fetch_tags_box), fetch_tags_icon);
  gtk_box_append(GTK_BOX(fetch_tags_box), gtk_label_new(Translations::CStr(EditTagSummaryLabels::FetchTags())));
  state->fetch_tags = gtk_button_new();
  gtk_button_set_child(GTK_BUTTON(state->fetch_tags), fetch_tags_box);
  gtk_box_append(GTK_BOX(tags_art_col), state->fetch_tags);
  gtk_box_append(GTK_BOX(tags_art_row), tags_art_col);
  gtk_box_append(GTK_BOX(tags), tags_art_row);
  AttachCoverGesture(state->tags_art, state, true);
  AttachCoverDrop(state->tags_art, state);
  const std::string tags_summary = EditTagCompleter::TagsSummary(static_cast<int>(targets.size()));
  if (!tags_summary.empty()) {
    state->tags_summary = gtk_label_new(tags_summary.c_str());
    gtk_widget_add_css_class(state->tags_summary, "dim-label");
    gtk_label_set_xalign(GTK_LABEL(state->tags_summary), 0.0f);
    gtk_box_append(GTK_BOX(tags), state->tags_summary);
  }
  add_entries(tags, {{"Title", EditTagFields::CommonValue(targets, [](const Song &s) { return s.title(); })},
                     {"Artist", EditTagFields::CommonValue(targets, [](const Song &s) { return s.artist(); })},
                     {"Album", EditTagFields::CommonValue(targets, [](const Song &s) { return s.album(); })},
                     {"Album artist", EditTagFields::CommonValue(targets, [](const Song &s) { return s.albumartist(); })},
                     {"Year", EditTagFields::CommonValue(targets, [](const Song &s) { return s.year() > 0 ? std::to_string(s.year()) : std::string(); })},
                     {"Original year",
                      EditTagFields::CommonValue(targets, [](const Song &s) { return s.originalyear() > 0 ? std::to_string(s.originalyear()) : std::string(); })},
                     {"Track", EditTagFields::CommonValue(targets, [](const Song &s) { return s.track() > 0 ? std::to_string(s.track()) : std::string(); })},
                     {"Genre", EditTagFields::CommonValue(targets, [](const Song &s) { return s.genre(); })}});
  GtkWidget *rating_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  state->rating_label = gtk_label_new(Translations::CStr("Rating"));
  gtk_label_set_xalign(GTK_LABEL(state->rating_label), 0);
  gtk_widget_set_hexpand(state->rating_label, TRUE);
  state->rating_reset = MakeFieldResetButton();
  gtk_widget_set_visible(state->rating_reset, FALSE);
  gtk_box_append(GTK_BOX(rating_header), state->rating_label);
  gtk_box_append(GTK_BOX(rating_header), state->rating_reset);
  gtk_box_append(GTK_BOX(tags), rating_header);
  state->rating = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 5, 0.5);
  gtk_scale_set_digits(GTK_SCALE(state->rating), 1);
  gtk_scale_set_draw_value(GTK_SCALE(state->rating), TRUE);
  state->initial_rating_stored = EditTagFields::CommonRating(targets);
  state->initial_rating = EditTagFields::RatingSliderFromStored(state->initial_rating_stored);
  gtk_range_set_value(GTK_RANGE(state->rating), state->initial_rating);
  g_object_set_data(G_OBJECT(state->rating), "state", state);
  g_signal_connect(state->rating, "value-changed", G_CALLBACK((+[](GtkRange *range, gpointer) {
                     auto *self = static_cast<State *>(g_object_get_data(G_OBJECT(range), "state"));
                     if (!self) {
                       return;
                     }
                     const bool modified = EditTagFields::IsRatingModified(self->initial_rating_stored,
                                                                          EditTagFields::RatingStoredFromSlider(gtk_range_get_value(range)));
                     SetModifiedStyle(self->rating_label, modified);
                     if (self->rating_reset) {
                       gtk_widget_set_visible(self->rating_reset, modified);
                     }
                   })),
                   nullptr);
  g_signal_connect(state->rating_reset, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self && self->rating) {
                       gtk_range_set_value(GTK_RANGE(self->rating), self->initial_rating);
                     }
                   }),
                   state);
  gtk_box_append(GTK_BOX(tags), state->rating);
  state->compilation = gtk_check_button_new_with_label(Translations::CStr("Compilation"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->compilation), state->song.compilation());
  gtk_box_append(GTK_BOX(tags), state->compilation);
  add_entries(tags, {{"Composer", EditTagFields::CommonValue(targets, [](const Song &s) { return s.composer(); })},
                     {"Performer", EditTagFields::CommonValue(targets, [](const Song &s) { return s.performer(); })},
                     {"Grouping", EditTagFields::CommonValue(targets, [](const Song &s) { return s.grouping(); })},
                     {"Comment", EditTagFields::CommonValue(targets, [](const Song &s) { return s.comment(); })},
                     {"Disc", EditTagFields::CommonValue(targets, [](const Song &s) { return s.disc() > 0 ? std::to_string(s.disc()) : std::string(); })},
                     {"BPM", EditTagFields::CommonValue(targets, [](const Song &s) { return s.bpm() > 0 ? std::to_string(s.bpm()) : std::string(); })},
                     {"Mood", EditTagFields::CommonValue(targets, [](const Song &s) { return s.mood(); })},
                     {"Initial key", EditTagFields::CommonValue(targets, [](const Song &s) { return s.initial_key(); })},
                     {"Title sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.titlesort(); })},
                     {"Artist sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.artistsort(); })},
                     {"Album sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.albumsort(); })},
                     {"Album artist sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.albumartistsort(); })},
                     {"Composer sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.composersort(); })},
                     {"Performer sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.performersort(); })}});
  adw_view_stack_add_titled(stack, tags, "Tags", Translations::CStr("Tags"));

  state->lyrics_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(state->lyrics_page, 12);
  gtk_widget_set_margin_end(state->lyrics_page, 12);
  gtk_widget_set_margin_top(state->lyrics_page, 12);
  state->lyrics = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->lyrics), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(state->lyrics, TRUE);
  const auto lyrics_common = EditTagFields::CommonValue(targets, [](const Song &s) { return s.lyrics(); });
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), lyrics_common.first.c_str(), -1);
  state->fields.emplace_back("Lyrics", state->lyrics);
  state->initial.push_back(lyrics_common.first);
  GtkWidget *lyrics_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  state->lyrics_label = gtk_label_new(Translations::CStr("Lyrics"));
  gtk_label_set_xalign(GTK_LABEL(state->lyrics_label), 0);
  gtk_widget_set_hexpand(state->lyrics_label, TRUE);
  state->lyrics_reset = MakeFieldResetButton();
  gtk_widget_set_visible(state->lyrics_reset, FALSE);
  gtk_box_append(GTK_BOX(lyrics_header), state->lyrics_label);
  gtk_box_append(GTK_BOX(lyrics_header), state->lyrics_reset);
  g_object_set_data(G_OBJECT(state->lyrics), "state", state);
  g_object_set_data_full(G_OBJECT(state->lyrics), "initial-text", g_strdup(lyrics_common.first.c_str()), g_free);
  g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), "changed", G_CALLBACK((+[](GtkTextBuffer *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (!self || !self->lyrics) {
                       return;
                     }
                     const char *initial_text = static_cast<const char *>(g_object_get_data(G_OBJECT(self->lyrics), "initial-text"));
                     const bool modified = EditTagFields::IsValueModified("Lyrics", initial_text ? initial_text : "", TextOf(self->lyrics));
                     SetModifiedStyle(self->lyrics_label, modified);
                     if (self->lyrics_reset) {
                       gtk_widget_set_visible(self->lyrics_reset, modified);
                     }
                   })),
                   state);
  g_signal_connect(state->lyrics_reset, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (!self || !self->lyrics) {
                       return;
                     }
                     const char *initial_text = static_cast<const char *>(g_object_get_data(G_OBJECT(self->lyrics), "initial-text"));
                     SetText(self->lyrics, initial_text ? initial_text : "");
                   }),
                   state);
  gtk_box_append(GTK_BOX(state->lyrics_page), lyrics_header);
  gtk_box_append(GTK_BOX(state->lyrics_page), state->lyrics);
  adw_view_stack_add_titled(stack, state->lyrics_page, "Lyrics", Translations::CStr("Lyrics"));

  GtkWidget *stats_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(stats_page, 12);
  gtk_widget_set_margin_end(stats_page, 12);
  gtk_widget_set_margin_top(stats_page, 12);
  GtkWidget *plays_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(plays_row), gtk_label_new(Translations::CStr("Play count")));
  state->stats_plays = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(state->stats_plays), 1);
  gtk_widget_set_hexpand(state->stats_plays, TRUE);
  gtk_box_append(GTK_BOX(plays_row), state->stats_plays);
  GtkWidget *skips_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(skips_row), gtk_label_new(Translations::CStr("Skip count")));
  state->stats_skips = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(state->stats_skips), 1);
  gtk_widget_set_hexpand(state->stats_skips, TRUE);
  gtk_box_append(GTK_BOX(skips_row), state->stats_skips);
  GtkWidget *last_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(last_row), gtk_label_new(Translations::CStr("Last played")));
  state->stats_last = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(state->stats_last), 1);
  gtk_widget_set_hexpand(state->stats_last, TRUE);
  gtk_box_append(GTK_BOX(last_row), state->stats_last);
  state->stats_path = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(state->stats_path), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->stats_path), 0);
  gtk_widget_add_css_class(state->stats_path, "dim-label");
  GtkWidget *reset_stats = gtk_button_new_with_label(Translations::CStr("Reset play statistics"));
  gtk_box_append(GTK_BOX(stats_page), plays_row);
  gtk_box_append(GTK_BOX(stats_page), skips_row);
  gtk_box_append(GTK_BOX(stats_page), last_row);
  gtk_box_append(GTK_BOX(stats_page), state->stats_path);
  gtk_box_append(GTK_BOX(stats_page), reset_stats);
  adw_view_stack_add_titled(stack, stats_page, "Statistics", Translations::CStr("Statistics"));

  state->actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(state->actions, 12);
  gtk_widget_set_margin_end(state->actions, 12);
  gtk_widget_set_margin_bottom(state->actions, 8);
  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save"));
  gtk_widget_add_css_class(save, "suggested-action");
  GtkWidget *reset_fields = gtk_button_new_with_label(Translations::CStr("Reset fields"));
  state->fetch_lyrics = gtk_button_new_with_label(Translations::CStr(EditTagSummaryLabels::FetchLyrics()));
  gtk_box_append(GTK_BOX(state->actions), save);
  gtk_box_append(GTK_BOX(state->actions), reset_fields);
  gtk_box_append(GTK_BOX(state->actions), state->fetch_lyrics);

  g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     std::vector<std::pair<std::string, std::string>> changed;
                     for (size_t i = 0; i < self->fields.size(); ++i) {
                       const std::string value = TextOf(self->fields[i].second);
                       const std::string initial = i < self->initial.size() ? self->initial[i] : std::string();
                       if (!EditTagFields::IsValueModified(self->fields[i].first, initial, value)) {
                         continue;
                       }
                       changed.emplace_back(self->fields[i].first, value);
                     }
                     EditTagFields::ApplyChangedFields(&self->songs, changed);
                     const bool write_compilation = self->compilation != nullptr;
                     const bool write_rating = self->rating != nullptr &&
                                               EditTagFields::IsRatingModified(self->initial_rating_stored,
                                                                               EditTagFields::RatingStoredFromSlider(gtk_range_get_value(GTK_RANGE(self->rating))));
                     const TagID3v2Version id3v2_version =
                         self->id3v2 ? EditTagId3v2::TagVersionFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(self->id3v2))))
                                     : TagID3v2Version::Default;
                     for (Song &song : self->songs) {
                       if (write_compilation) {
                         song.set_compilation(gtk_check_button_get_active(GTK_CHECK_BUTTON(self->compilation)));
                       }
                       if (write_rating) {
                         song.set_rating(EditTagFields::RatingStoredFromSlider(gtk_range_get_value(GTK_RANGE(self->rating))));
                       }
                       EditTagFields::NormalizeUnsetNumeric(&song);
                       self->app->tagreader()->WriteFile(FileUtils::PathFromUri(song.url()), song, static_cast<int>(SaveTagsOption::Tags), {},
                                                         id3v2_version);
                       const std::string path = FileUtils::PathFromUri(song.url());
                       if (!path.empty() && song.rating() >= 0) {
                         self->app->tagreader()->SaveRating(path, song.rating());
                       }
                       if (song.id() > 0) {
                         self->app->collection()->backend()->AddOrUpdateSong(song);
                         self->app->collection()->backend()->SetRating(song.id(), song.rating());
                       }
                     }
                     if (self->index < self->songs.size()) {
                       self->song = self->songs[self->index];
                     }
                     gtk_button_set_label(button, Translations::CStr("Saved"));
                   })),
                   state);

  g_signal_connect(reset_fields, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     for (size_t i = 0; i < self->fields.size() && i < self->initial.size(); ++i) {
                       SetText(self->fields[i].second, self->initial[i]);
                     }
                     if (self->rating) {
                       gtk_range_set_value(GTK_RANGE(self->rating), self->initial_rating);
                     }
                   })),
                   state);

  g_signal_connect(state->fetch_tags, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     TrackSelectionDialog::Show(self->parent, self->app, self->songs);
                   })),
                   state);

  g_signal_connect(state->fetch_lyrics, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     self->app->lyrics_providers()->Fetch(self->song, [button, self](const std::string &lyrics, const std::string &) {
                       if (lyrics.empty()) {
                         gtk_button_set_label(button, Translations::CStr("Missing"));
                         return;
                       }
                       self->song.set_lyrics(lyrics);
                       if (self->index < self->songs.size()) {
                         self->songs[self->index].set_lyrics(lyrics);
                       }
                       if (self->lyrics) {
                         gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->lyrics)), lyrics.c_str(), -1);
                       }
                       gtk_button_set_label(button, Translations::CStr("Fetched"));
                     });
                   })),
                   state);

  g_signal_connect(reset_stats, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self->index >= self->songs.size()) {
                       return;
                     }
                     EditTagFields::ResetPlayStatistics(&self->songs[self->index]);
                     PersistPlayStatistics(self, &self->songs[self->index]);
                     UpdateDisplay(self);
                     gtk_button_set_label(button, Translations::CStr("Reset"));
                   })),
                   state);

  auto connect_cover = [&](GtkWidget *button, const char *action) {
    g_object_set_data(G_OBJECT(button), "cover-action", const_cast<char *>(action));
    g_signal_connect(button, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<State *>(data);
                       const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "cover-action"));
                       if (g_strcmp0(name, "fetch") == 0) self->covers->FetchCover(&self->song, self->cover, GTK_WIDGET(btn));
                       else if (g_strcmp0(name, "search") == 0) self->covers->SearchForCover(self->parent);
                       else if (g_strcmp0(name, "url") == 0) self->covers->LoadCoverFromURL(self->parent, &self->song, self->cover);
                       else if (g_strcmp0(name, "file") == 0) self->covers->LoadCoverFromFile(self->parent, &self->song, self->cover);
                       else if (g_strcmp0(name, "unset") == 0) self->covers->UnsetCover(&self->song, self->cover);
                       else if (g_strcmp0(name, "clear") == 0) self->covers->ClearCover(&self->song, self->cover);
                       else if (g_strcmp0(name, "delete") == 0) self->covers->DeleteCover(&self->song, self->cover);
                       else if (g_strcmp0(name, "show") == 0) self->covers->ShowCover(self->parent, self->song);
                       else if (g_strcmp0(name, "save") == 0) self->covers->SaveCoverToFile(self->parent, self->song);
                       else if (g_strcmp0(name, "stats") == 0) self->covers->ShowStatistics(self->parent);
                       if (self->index < self->songs.size()) {
                         self->songs[self->index] = self->song;
                       }
                       UpdateDisplay(self);
                     })),
                     state);
  };
  if (state->tags_art_button) {
    if (auto *menu_box = static_cast<GtkWidget *>(g_object_get_data(G_OBJECT(state->tags_art_button), "menu-box"))) {
      auto add_menu = [&](const char *label, const char *action) {
        GtkWidget *item = gtk_button_new_with_label(Translations::CStr(label));
        gtk_widget_add_css_class(item, "flat");
        connect_cover(item, action);
        gtk_box_append(GTK_BOX(menu_box), item);
      };
      add_menu("Fetch cover", "fetch");
      add_menu("Search…", "search");
      add_menu("From URL", "url");
      add_menu("From file", "file");
      add_menu("Unset", "unset");
      add_menu("Clear", "clear");
      add_menu("Delete", "delete");
      add_menu("Show", "show");
      add_menu("Save…", "save");
    }
  }
  connect_cover(state->fetch_cover, "fetch");
  connect_cover(state->search_cover, "search");
  connect_cover(state->url_cover, "url");
  connect_cover(state->file_cover, "file");
  connect_cover(state->unset_cover, "unset");
  connect_cover(state->clear_cover, "clear");
  connect_cover(state->delete_cover, "delete");
  connect_cover(state->show_cover, "show");
  connect_cover(state->save_cover, "save");
  connect_cover(stats_cover, "stats");

  gtk_box_append(GTK_BOX(box), switcher);
  gtk_widget_set_vexpand(GTK_WIDGET(stack), TRUE);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(stack));
  {
    Settings tab_settings;
    tab_settings.BeginGroup(EditTagDialogSettings::kSettingsGroup);
    adw_view_stack_set_visible_child_name(
        stack, EditTagTabs::Name(EditTagTabs::VisibleIndex(tab_settings.IntValue(EditTagDialogSettings::kCurrentTab), targets.size())));
  }
  g_signal_connect(stack, "notify::visible-child-name", G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer) {
                     const char *name = adw_view_stack_get_visible_child_name(ADW_VIEW_STACK(object));
                     Settings tab_settings;
                     tab_settings.BeginGroup(EditTagDialogSettings::kSettingsGroup);
                     tab_settings.SetIntValue(EditTagDialogSettings::kCurrentTab, EditTagTabs::IndexFromName(name ? name : ""));
                     tab_settings.Sync();
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), state->actions);
  adw_dialog_set_child(dialog, box);
  state->dialog = dialog;
  SetLoading(state, true);
  UpdateDisplay(state);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  StartTagLoad(state, targets);
}
