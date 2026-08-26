#include "dialogs/edittagdialog.h"

#include "constants/edittagdialogsettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "dialogs/dialoghelpers.h"
#include "dialogs/dialoglistkeyboard.h"
#include "dialogs/edittagcover.h"
#include "dialogs/edittagcoverdrop.h"
#include "dialogs/edittagfields.h"
#include "dialogs/edittagid3v2.h"
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
  GtkWidget *lyrics = nullptr;
  GtkWidget *rating = nullptr;
  GtkWidget *compilation = nullptr;
  GtkWidget *stats_label = nullptr;
  GtkWidget *stats_plays = nullptr;
  GtkWidget *stats_skips = nullptr;
  GtkWidget *stats_last = nullptr;
  GtkWidget *stats_path = nullptr;
  GtkWidget *song_list = nullptr;
  GtkWidget *prev = nullptr;
  GtkWidget *next = nullptr;
  GtkWidget *id3v2 = nullptr;
  GtkWidget *embedded_cover = nullptr;
  std::unique_ptr<AlbumCoverChoiceController> covers;
  std::vector<std::pair<std::string, GtkWidget *>> fields;
  std::vector<std::string> initial;
};

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

void UpdateDisplay(State *state) {
  if (!state || state->index >= state->songs.size()) {
    return;
  }
  state->song = state->songs[state->index];
  if (state->cover && state->app) {
    SetImageFromBytes(state->cover, state->app->albumcover_loader()->LoadData(state->song), 160);
  }
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
  if (state->prev) {
    gtk_widget_set_sensitive(state->prev, state->songs.size() > 1);
  }
  if (state->next) {
    gtk_widget_set_sensitive(state->next, state->songs.size() > 1);
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
}

void SelectSong(State *state, int index) {
  if (!state || state->songs.empty()) {
    return;
  }
  state->index = static_cast<size_t>(EditTagFields::WrapIndex(index, 0, static_cast<int>(state->songs.size())));
  UpdateDisplay(state);
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
  SongList targets = songs;
  if (targets.empty()) {
    targets.push_back(SongForDialog(app));
  }
  auto *state = new State();
  state->app = app;
  state->parent = parent;
  state->songs = targets;
  state->index = 0;
  state->song = targets.front();
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

  if (targets.size() > 1) {
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
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), stack);

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
      gtk_box_append(GTK_BOX(page), GTK_WIDGET(entry));
    }
  };

  GtkWidget *summary = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(summary, 12);
  gtk_widget_set_margin_end(summary, 12);
  gtk_widget_set_margin_top(summary, 12);
  state->cover = gtk_image_new();
  gtk_widget_set_halign(state->cover, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(summary), state->cover);
  GtkDropTarget *cover_drop = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_widget_add_controller(state->cover, GTK_EVENT_CONTROLLER(cover_drop));
  g_signal_connect(cover_drop, "drop",
                   G_CALLBACK((+[](GtkDropTarget *, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
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
  GtkWidget *cover_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign(cover_buttons, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(cover_buttons, TRUE);
  GtkWidget *cover_buttons2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign(cover_buttons2, GTK_ALIGN_CENTER);
  GtkWidget *fetch_cover = gtk_button_new_with_label(Translations::CStr("Fetch cover"));
  GtkWidget *search_cover = gtk_button_new_with_label(Translations::CStr("Search…"));
  GtkWidget *url_cover = gtk_button_new_with_label(Translations::CStr("From URL"));
  GtkWidget *file_cover = gtk_button_new_with_label(Translations::CStr("From file"));
  GtkWidget *unset_cover = gtk_button_new_with_label(Translations::CStr("Unset"));
  GtkWidget *clear_cover = gtk_button_new_with_label(Translations::CStr("Clear"));
  GtkWidget *delete_cover = gtk_button_new_with_label(Translations::CStr("Delete"));
  GtkWidget *show_cover = gtk_button_new_with_label(Translations::CStr("Show"));
  GtkWidget *save_cover = gtk_button_new_with_label(Translations::CStr("Save…"));
  GtkWidget *stats_cover = gtk_button_new_with_label(Translations::CStr("Statistics"));
  gtk_box_append(GTK_BOX(cover_buttons), fetch_cover);
  gtk_box_append(GTK_BOX(cover_buttons), search_cover);
  gtk_box_append(GTK_BOX(cover_buttons), url_cover);
  gtk_box_append(GTK_BOX(cover_buttons), file_cover);
  gtk_box_append(GTK_BOX(cover_buttons), unset_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), clear_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), delete_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), show_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), save_cover);
  gtk_box_append(GTK_BOX(cover_buttons2), stats_cover);
  gtk_box_append(GTK_BOX(summary), cover_buttons);
  gtk_box_append(GTK_BOX(summary), cover_buttons2);
  state->embedded_cover = gtk_check_button_new_with_label(Translations::CStr("Embedded cover"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->embedded_cover),
                              EditTagCover::DefaultEmbeddedChecked(state->song, state->covers->get_collection_save_album_cover_type()));
  gtk_widget_set_sensitive(state->embedded_cover, EditTagCover::AnySupported(targets));
  gtk_box_append(GTK_BOX(summary), state->embedded_cover);
  g_signal_connect(state->embedded_cover, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self->covers) {
                       self->covers->set_save_embedded_cover_override(gtk_check_button_get_active(button));
                     }
                   }),
                   state);
  state->stats_label = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(state->stats_label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->stats_label), 0);
  gtk_widget_add_css_class(state->stats_label, "dim-label");
  gtk_box_append(GTK_BOX(summary), state->stats_label);
  GtkWidget *rating_label = gtk_label_new(Translations::CStr("Rating"));
  gtk_label_set_xalign(GTK_LABEL(rating_label), 0);
  gtk_box_append(GTK_BOX(summary), rating_label);
  state->rating = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 5, 0.5);
  gtk_scale_set_digits(GTK_SCALE(state->rating), 1);
  gtk_scale_set_draw_value(GTK_SCALE(state->rating), TRUE);
  gtk_range_set_value(GTK_RANGE(state->rating), state->song.rating() >= 0 ? state->song.rating() * 5.0 : 0);
  gtk_box_append(GTK_BOX(summary), state->rating);
  add_entries(summary, {{"Title", EditTagFields::CommonValue(targets, [](const Song &s) { return s.title(); })},
                        {"Artist", EditTagFields::CommonValue(targets, [](const Song &s) { return s.artist(); })},
                        {"Album", EditTagFields::CommonValue(targets, [](const Song &s) { return s.album(); })},
                        {"Album artist", EditTagFields::CommonValue(targets, [](const Song &s) { return s.albumartist(); })},
                        {"Year", EditTagFields::CommonValue(targets, [](const Song &s) { return s.year() > 0 ? std::to_string(s.year()) : std::string(); })},
                        {"Original year",
                         EditTagFields::CommonValue(targets, [](const Song &s) { return s.originalyear() > 0 ? std::to_string(s.originalyear()) : std::string(); })},
                        {"Track", EditTagFields::CommonValue(targets, [](const Song &s) { return s.track() > 0 ? std::to_string(s.track()) : std::string(); })},
                        {"Genre", EditTagFields::CommonValue(targets, [](const Song &s) { return s.genre(); })}});
  adw_view_stack_add_titled(stack, summary, "Summary", Translations::CStr("Summary"));

  GtkWidget *tags = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(tags, 12);
  gtk_widget_set_margin_end(tags, 12);
  gtk_widget_set_margin_top(tags, 12);
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
                     {"Album artist sort", EditTagFields::CommonValue(targets, [](const Song &s) { return s.albumartistsort(); })}});
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
    gtk_box_append(GTK_BOX(tags), id3_row);
  }
  adw_view_stack_add_titled(stack, tags, "Tags", Translations::CStr("Tags"));

  GtkWidget *lyrics_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(lyrics_page, 12);
  gtk_widget_set_margin_end(lyrics_page, 12);
  gtk_widget_set_margin_top(lyrics_page, 12);
  state->lyrics = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->lyrics), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(state->lyrics, TRUE);
  const auto lyrics_common = EditTagFields::CommonValue(targets, [](const Song &s) { return s.lyrics(); });
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), lyrics_common.first.c_str(), -1);
  state->fields.emplace_back("Lyrics", state->lyrics);
  state->initial.push_back(lyrics_common.first);
  gtk_box_append(GTK_BOX(lyrics_page), gtk_label_new(Translations::CStr("Lyrics")));
  gtk_box_append(GTK_BOX(lyrics_page), state->lyrics);
  adw_view_stack_add_titled(stack, lyrics_page, "Lyrics", Translations::CStr("Lyrics"));

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

  GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(actions, 12);
  gtk_widget_set_margin_end(actions, 12);
  gtk_widget_set_margin_bottom(actions, 8);
  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save"));
  gtk_widget_add_css_class(save, "suggested-action");
  GtkWidget *reset_fields = gtk_button_new_with_label(Translations::CStr("Reset fields"));
  GtkWidget *fetch_tags = gtk_button_new_with_label(Translations::CStr("Fetch tags"));
  GtkWidget *fetch_lyrics = gtk_button_new_with_label(Translations::CStr("Fetch lyrics"));
  gtk_box_append(GTK_BOX(actions), save);
  gtk_box_append(GTK_BOX(actions), reset_fields);
  gtk_box_append(GTK_BOX(actions), fetch_tags);
  gtk_box_append(GTK_BOX(actions), fetch_lyrics);

  g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     std::vector<std::pair<std::string, std::string>> changed;
                     for (size_t i = 0; i < self->fields.size(); ++i) {
                       const std::string value = TextOf(self->fields[i].second);
                       const std::string initial = i < self->initial.size() ? self->initial[i] : std::string();
                       if (value == initial) {
                         continue;
                       }
                       changed.emplace_back(self->fields[i].first, value);
                     }
                     EditTagFields::ApplyChangedFields(&self->songs, changed);
                     const bool write_compilation = self->compilation != nullptr;
                     const bool write_rating = self->rating != nullptr;
                     const TagID3v2Version id3v2_version =
                         self->id3v2 ? EditTagId3v2::TagVersionFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(self->id3v2))))
                                     : TagID3v2Version::Default;
                     for (Song &song : self->songs) {
                       if (write_compilation) {
                         song.set_compilation(gtk_check_button_get_active(GTK_CHECK_BUTTON(self->compilation)));
                       }
                       if (write_rating) {
                         song.set_rating(static_cast<float>(gtk_range_get_value(GTK_RANGE(self->rating)) / 5.0));
                       }
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
                   })),
                   state);

  g_signal_connect(fetch_tags, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     TrackSelectionDialog::Show(self->parent, self->app, self->songs);
                   })),
                   state);

  g_signal_connect(fetch_lyrics, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
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
                     })),
                     state);
  };
  connect_cover(fetch_cover, "fetch");
  connect_cover(search_cover, "search");
  connect_cover(url_cover, "url");
  connect_cover(file_cover, "file");
  connect_cover(unset_cover, "unset");
  connect_cover(clear_cover, "clear");
  connect_cover(delete_cover, "delete");
  connect_cover(show_cover, "show");
  connect_cover(save_cover, "save");
  connect_cover(stats_cover, "stats");

  gtk_box_append(GTK_BOX(box), switcher);
  gtk_widget_set_vexpand(GTK_WIDGET(stack), TRUE);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(stack));
  {
    Settings tab_settings;
    tab_settings.BeginGroup(EditTagDialogSettings::kSettingsGroup);
    adw_view_stack_set_visible_child_name(stack, EditTagTabs::Name(tab_settings.IntValue(EditTagDialogSettings::kCurrentTab)));
  }
  g_signal_connect(stack, "notify::visible-child-name", G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer) {
                     const char *name = adw_view_stack_get_visible_child_name(ADW_VIEW_STACK(object));
                     Settings tab_settings;
                     tab_settings.BeginGroup(EditTagDialogSettings::kSettingsGroup);
                     tab_settings.SetIntValue(EditTagDialogSettings::kCurrentTab, EditTagTabs::IndexFromName(name ? name : ""));
                     tab_settings.Sync();
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), actions);
  adw_dialog_set_child(dialog, box);
  UpdateDisplay(state);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
