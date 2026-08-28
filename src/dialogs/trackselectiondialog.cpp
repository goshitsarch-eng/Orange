#include "dialogs/trackselectiondialog.h"
#include "dialogs/dialogchrome.h"

#include "core/application.h"
#include "dialogs/dialoglistkeyboard.h"
#include "dialogs/dialogsongnav.h"
#include "dialogs/trackselectionlabels.h"
#include "dialogs/uierror.h"
#include "playlist/playlistmanager.h"
#include "tagfetcher/tagfetcher.h"
#include "tagfetcher/tagfetchhelpers.h"
#include "translations/translations.h"
#include "widgets/listboxkeyboardgtk.h"

#include <adwaita.h>

#include <memory>
#include <string>
#include <vector>

namespace {

struct PendingSong {
  int fetch_id = 0;
  Song original;
  SongList results;
  int selected = 0;
  std::string status;
  bool applied = false;
};

struct State {
  Application *app = nullptr;
  std::unique_ptr<TagFetcher> fetcher;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
  std::vector<PendingSong> songs;
  int current = 0;
  int completed = 0;
  GtkWidget *status = nullptr;
  GtkWidget *progress = nullptr;
  GtkWidget *song_list = nullptr;
  GtkWidget *song_pane = nullptr;
  GtkWidget *results = nullptr;
  GtkWidget *prev = nullptr;
  GtkWidget *next = nullptr;
  GtkWidget *apply = nullptr;
  GtkWidget *apply_all = nullptr;
  GtkWidget *paned = nullptr;
  GtkWidget *buttons = nullptr;
  GtkWidget *loading = nullptr;

  ~State() {
    *alive = false;
    if (fetcher) {
      fetcher->Cancel();
    }
  }
};

std::string SongSubtitle(const Song &song) {
  std::string text = song.artist();
  if (!song.album().empty()) {
    if (!text.empty()) {
      text += " – ";
    }
    text += song.album();
  }
  if (song.year() > 0) {
    text += " (" + std::to_string(song.year()) + ")";
  }
  return text;
}

PendingSong *FindById(State *state, int fetch_id) {
  for (PendingSong &song : state->songs) {
    if (song.fetch_id == fetch_id) {
      return &song;
    }
  }
  return nullptr;
}

void PersistSong(Application *app, const Song &song) {
  if (!app) {
    return;
  }
  if (app->tagreader() && (!song.url().empty() || song.is_valid())) {
    app->tagreader()->WriteFile(song);
  }
  if (song.id() > 0 || song.is_collection_song()) {
    app->collection()->backend()->AddOrUpdateSong(song);
  }
  if (app->playlist_manager()) {
    for (Playlist *playlist : app->playlist_manager()->GetAllPlaylists()) {
      playlist->UpdateSongsByUrl(song);
    }
  }
}

void RefreshResults(State *state);
void RefreshSongList(State *state);

void SelectSong(State *state, int index) {
  if (!state || state->songs.empty()) {
    return;
  }
  if (index < 0) {
    index = 0;
  }
  if (index >= static_cast<int>(state->songs.size())) {
    index = static_cast<int>(state->songs.size()) - 1;
  }
  state->current = index;
  const bool nav = TrackSelectionLabels::NavEnabled(static_cast<int>(state->songs.size()));
  if (state->prev) {
    gtk_widget_set_sensitive(state->prev, nav);
  }
  if (state->next) {
    gtk_widget_set_sensitive(state->next, nav);
  }
  RefreshSongList(state);
  RefreshResults(state);
}

void RefreshSongList(State *state) {
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
    const PendingSong &pending = state->songs[i];
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), pending.original.PrettyTitle().c_str());
    std::string subtitle = pending.status.empty() ? SongSubtitle(pending.original) : pending.status;
    if (pending.applied) {
      subtitle = Translations::Tr("Applied");
    }
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle.c_str());
    if (static_cast<int>(i) == state->current) {
      gtk_widget_add_css_class(row, "accent");
    }
    g_object_set_data(G_OBJECT(row), "song-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    gtk_list_box_append(GTK_LIST_BOX(state->song_list), row);
  }
}

void RefreshResults(State *state) {
  if (!state || !state->results || state->songs.empty()) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(state->results);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(state->results), child);
    child = next;
  }
  PendingSong &pending = state->songs[static_cast<size_t>(state->current)];
  if (pending.results.empty()) {
    GtkWidget *empty = adw_action_row_new();
    const bool no_results = TrackSelectionLabels::ShowEmptyResults(pending.status != Translations::Tr("No matches"), false);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(empty),
                                  no_results ? TrackSelectionLabels::NoResults() : Translations::CStr("Waiting for results…"));
    if (no_results) {
      adw_action_row_set_subtitle(ADW_ACTION_ROW(empty), TrackSelectionLabels::UnableToFind());
    } else if (!pending.status.empty()) {
      adw_action_row_set_subtitle(ADW_ACTION_ROW(empty), pending.status.c_str());
    }
    gtk_list_box_append(GTK_LIST_BOX(state->results), empty);
    if (state->apply) {
      gtk_widget_set_sensitive(state->apply, FALSE);
    }
    return;
  }
  for (size_t i = 0; i < pending.results.size(); ++i) {
    const Song &result = pending.results[i];
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), result.PrettyTitle().c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), SongSubtitle(result).c_str());
    if (static_cast<int>(i) == pending.selected) {
      gtk_widget_add_css_class(row, "accent");
    }
    g_object_set_data(G_OBJECT(row), "result-index", GINT_TO_POINTER(static_cast<int>(i + 1)));
    gtk_list_box_append(GTK_LIST_BOX(state->results), row);
  }
  if (state->apply) {
    gtk_widget_set_sensitive(state->apply, pending.applied ? FALSE : TRUE);
  }
}

void SetLoading(State *state, bool loading) {
  if (!state) {
    return;
  }
  if (state->paned) {
    gtk_widget_set_sensitive(state->paned, TrackSelectionLabels::SplitterEnabled(loading));
  }
  if (state->buttons) {
    gtk_widget_set_sensitive(state->buttons, TrackSelectionLabels::ButtonsEnabled(loading));
  }
  if (state->loading) {
    gtk_widget_set_visible(state->loading, TrackSelectionLabels::LoadingVisible(loading));
    if (loading) {
      gtk_label_set_text(GTK_LABEL(state->loading), Translations::CStr(TrackSelectionLabels::SavingTracks()));
    }
  }
}

void UpdateProgress(State *state) {
  if (!state) {
    return;
  }
  const int total = static_cast<int>(state->songs.size());
  const TagFetchHelpers::BatchProgress progress = TagFetchHelpers::BatchProgress::FromCounts(state->completed, total);
  if (state->progress) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), progress.Fraction());
  }
  if (state->status) {
    if (progress.Done()) {
      gtk_label_set_text(GTK_LABEL(state->status), Translations::CStr("Tag search finished"));
    } else {
      gtk_label_set_text(GTK_LABEL(state->status), (Translations::Tr("Fetching tags ") + progress.StatusText()).c_str());
    }
  }
  int applyable = 0;
  for (const PendingSong &pending : state->songs) {
    if (!pending.results.empty() && !pending.applied) {
      ++applyable;
    }
  }
  if (state->apply_all) {
    gtk_widget_set_sensitive(state->apply_all, applyable > 0 ? TRUE : FALSE);
  }
}

void ApplyPending(State *state, PendingSong *pending) {
  if (!state || !pending || pending->results.empty() || pending->applied) {
    return;
  }
  if (pending->selected < 0 || pending->selected >= static_cast<int>(pending->results.size())) {
    pending->selected = 0;
  }
  const Song written = TagFetchHelpers::ApplyTags(pending->original, pending->results[static_cast<size_t>(pending->selected)]);
  PersistSong(state->app, written);
  pending->original = written;
  pending->applied = true;
  pending->status = Translations::Tr("Applied");
}

}  // namespace

void TrackSelectionDialog::Show(GtkWindow *parent, Application *app, const SongList &songs) {
  SongList targets = songs;
  if (targets.empty() && app && app->player()) {
    const Song current = app->player()->current_song();
    if (current.is_valid() || !current.url().empty()) {
      targets.push_back(current);
    }
  }
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(TrackSelectionLabels::Title()));
  adw_dialog_set_content_width(dialog, 720);
  adw_dialog_set_content_height(dialog, 560);

  auto *state = new State();
  state->app = app;
  g_object_set_data_full(G_OBJECT(dialog), "state", state, [](gpointer p) { delete static_cast<State *>(p); });

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  state->status = gtk_label_new(Translations::CStr("Searching AcoustID and MusicBrainz…"));
  gtk_label_set_wrap(GTK_LABEL(state->status), TRUE);
  gtk_widget_set_halign(state->status, GTK_ALIGN_START);
  state->progress = gtk_progress_bar_new();
  gtk_box_append(GTK_BOX(box), state->status);
  gtk_box_append(GTK_BOX(box), state->progress);
  state->loading = gtk_label_new(Translations::CStr(TrackSelectionLabels::SavingTracks()));
  gtk_widget_add_css_class(state->loading, "dim-label");
  gtk_widget_set_halign(state->loading, GTK_ALIGN_START);
  gtk_widget_set_visible(state->loading, TrackSelectionLabels::LoadingVisible(false));
  gtk_box_append(GTK_BOX(box), state->loading);

  GtkWidget *paned = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_hexpand(paned, TRUE);
  gtk_widget_set_vexpand(paned, TRUE);
  GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_hexpand(left, TRUE);
  GtkWidget *left_label = gtk_label_new(Translations::CStr("Songs"));
  gtk_widget_set_halign(left_label, GTK_ALIGN_START);
  gtk_widget_add_css_class(left_label, "heading");
  state->song_list = gtk_list_box_new();
  gtk_widget_add_css_class(state->song_list, "boxed-list");
  GtkWidget *left_scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(left_scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(left_scroll), state->song_list);
  gtk_box_append(GTK_BOX(left), left_label);
  gtk_box_append(GTK_BOX(left), left_scroll);
  state->song_pane = left;

  GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_hexpand(right, TRUE);
  GtkWidget *right_label = gtk_label_new(Translations::CStr(TrackSelectionLabels::SelectBest()));
  gtk_widget_set_halign(right_label, GTK_ALIGN_START);
  gtk_widget_add_css_class(right_label, "heading");
  state->results = gtk_list_box_new();
  gtk_widget_add_css_class(state->results, "boxed-list");
  GtkWidget *right_scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(right_scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(right_scroll), state->results);
  gtk_box_append(GTK_BOX(right), right_label);
  gtk_box_append(GTK_BOX(right), right_scroll);
  gtk_box_append(GTK_BOX(paned), left);
  gtk_box_append(GTK_BOX(paned), right);
  gtk_box_append(GTK_BOX(box), paned);
  state->paned = paned;

  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  state->prev = gtk_button_new_with_label(Translations::CStr("Previous"));
  state->next = gtk_button_new_with_label(Translations::CStr("Next"));
  state->apply = gtk_button_new_with_label(Translations::CStr("Apply"));
  state->apply_all = gtk_button_new_with_label(Translations::CStr("Apply all"));
  gtk_widget_add_css_class(state->apply, "suggested-action");
  gtk_widget_set_hexpand(buttons, TRUE);
  gtk_box_append(GTK_BOX(buttons), state->prev);
  gtk_box_append(GTK_BOX(buttons), state->next);
  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(buttons), spacer);
  gtk_box_append(GTK_BOX(buttons), state->apply_all);
  gtk_box_append(GTK_BOX(buttons), state->apply);
  gtk_box_append(GTK_BOX(box), buttons);
  state->buttons = buttons;

  DialogChrome::SetContent(dialog, box);
  if (state->prev && state->next) {
    GtkEventController *nav_keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(nav_keys, GTK_PHASE_CAPTURE);
    gtk_widget_add_controller(box, nav_keys);
    g_signal_connect(nav_keys, "key-pressed",
                     G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                       auto *self = static_cast<State *>(data);
                       const int delta = DialogSongNav::Delta(keyval, static_cast<unsigned>(state));
                       if (delta == 0 || !TrackSelectionLabels::NavEnabled(static_cast<int>(self->songs.size()))) {
                         return FALSE;
                       }
                       SelectSong(self, self->current + delta);
                       return TRUE;
                     })),
                     state);
  }
  adw_dialog_present(dialog, GTK_WIDGET(parent));

  const int song_count = static_cast<int>(targets.size());
  if (state->song_pane) {
    gtk_widget_set_visible(state->song_pane, TrackSelectionLabels::SongListVisible(song_count));
  }
  if (state->apply_all) {
    gtk_widget_set_visible(state->apply_all, TrackSelectionLabels::ApplyAllVisible(song_count));
  }
  if (state->prev) {
    gtk_widget_set_sensitive(state->prev, TrackSelectionLabels::NavEnabled(song_count));
  }
  if (state->next) {
    gtk_widget_set_sensitive(state->next, TrackSelectionLabels::NavEnabled(song_count));
  }
  if (targets.empty()) {
    gtk_label_set_text(GTK_LABEL(state->status), Translations::CStr("No song selected"));
    gtk_widget_set_sensitive(state->apply, FALSE);
    gtk_widget_set_sensitive(state->apply_all, FALSE);
    return;
  }

  g_signal_connect(state->prev, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     SelectSong(self, self->current - 1);
                   })),
                   state);
  g_signal_connect(state->next, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     SelectSong(self, self->current + 1);
                   })),
                   state);
  g_signal_connect(state->song_list, "row-activated", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "song-index")) - 1;
                     SelectSong(static_cast<State *>(data), index);
                   })),
                   state);
  g_signal_connect(state->results, "row-activated", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self->songs.empty()) {
                       return;
                     }
                     const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "result-index")) - 1;
                     self->songs[static_cast<size_t>(self->current)].selected = index;
                     RefreshResults(self);
                   })),
                   state);
  gtk_widget_set_focusable(state->song_list, TRUE);
  gtk_widget_set_focusable(state->results, TRUE);
  GtkEventController *song_keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(song_keys, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(state->song_list, song_keys);
  g_signal_connect(song_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     auto *self = static_cast<State *>(data);
                     if (DialogListKeyboard::IsActivate(keyval)) {
                       ListBoxKeyboardGtk::ActivateSelected(self->song_list);
                       return TRUE;
                     }
                     if (DialogListKeyboard::IsMove(keyval)) {
                       SelectSong(self, DialogListKeyboard::NextIndex(self->current, static_cast<int>(self->songs.size()), keyval));
                       return TRUE;
                     }
                     return FALSE;
                   })),
                   state);
  GtkEventController *result_keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(result_keys, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(state->results, result_keys);
  g_signal_connect(result_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     auto *self = static_cast<State *>(data);
                     if (self->songs.empty()) {
                       return FALSE;
                     }
                     PendingSong &pending = self->songs[static_cast<size_t>(self->current)];
                     if (DialogListKeyboard::IsActivate(keyval)) {
                       ListBoxKeyboardGtk::ActivateSelected(self->results);
                       return TRUE;
                     }
                     if (DialogListKeyboard::IsMove(keyval) && !pending.results.empty()) {
                       pending.selected = DialogListKeyboard::NextIndex(pending.selected, static_cast<int>(pending.results.size()), keyval);
                       RefreshResults(self);
                       return TRUE;
                     }
                     return FALSE;
                   })),
                   state);
  g_signal_connect(state->apply, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self->songs.empty()) {
                       return;
                     }
                     SetLoading(self, true);
                     ApplyPending(self, &self->songs[static_cast<size_t>(self->current)]);
                     SetLoading(self, false);
                     RefreshSongList(self);
                     RefreshResults(self);
                     UpdateProgress(self);
                   })),
                   state);
  g_signal_connect(state->apply_all, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     SetLoading(self, true);
                     for (PendingSong &pending : self->songs) {
                       ApplyPending(self, &pending);
                     }
                     SetLoading(self, false);
                     RefreshSongList(self);
                     RefreshResults(self);
                     UpdateProgress(self);
                   })),
                   state);

  state->fetcher = std::make_unique<TagFetcher>(app->network());
  const std::shared_ptr<bool> alive = state->alive;
  state->fetcher->Progress.Connect([alive, state](int id, const std::string &message) {
    if (!*alive) {
      return;
    }
    if (PendingSong *pending = FindById(state, id)) {
      pending->status = message;
    }
    UpdateProgress(state);
    RefreshSongList(state);
    if (!state->songs.empty() && state->songs[static_cast<size_t>(state->current)].fetch_id == id) {
      RefreshResults(state);
    }
  });
  state->fetcher->SongResults.Connect([alive, state](int id, const SongList &results) {
    if (!*alive) {
      return;
    }
    if (PendingSong *pending = FindById(state, id)) {
      pending->results = results;
      pending->selected = 0;
      pending->status = results.empty() ? Translations::Tr("No matches")
                                        : (std::to_string(results.size()) + Translations::Tr(" matches"));
    }
    ++state->completed;
    UpdateProgress(state);
    RefreshSongList(state);
    if (!state->songs.empty() && state->songs[static_cast<size_t>(state->current)].fetch_id == id) {
      RefreshResults(state);
    }
  });
  state->fetcher->Error.Connect([alive](int, const std::string &message) {
    if (*alive) {
      UiError::Report(message);
    }
  });
  state->fetcher->Finished.Connect([alive, state]() {
    if (!*alive) {
      return;
    }
    state->completed = static_cast<int>(state->songs.size());
    UpdateProgress(state);
  });

  const std::vector<int> ids = state->fetcher->QueueSongs(targets);
  for (size_t i = 0; i < targets.size(); ++i) {
    PendingSong pending;
    pending.original = targets[i];
    pending.status = Translations::Tr("Queued");
    pending.fetch_id = i < ids.size() ? ids[i] : 0;
    state->songs.push_back(pending);
  }
  SelectSong(state, 0);
  UpdateProgress(state);
  state->fetcher->Start();
}
