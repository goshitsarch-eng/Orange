#include "covermanager/albumcoversearcher.h"

#include "core/application.h"
#include "covermanager/albumcoverfetcher.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <memory>

using DialogHelpers::ApplyCover;
using DialogHelpers::SetImageFromBytes;
using DialogHelpers::SongForDialog;

namespace {

struct SearcherState {
  Application *app = nullptr;
  Song song;
  GtkWidget *artist = nullptr;
  GtkWidget *album = nullptr;
  GtkWidget *title = nullptr;
  GtkWidget *status = nullptr;
  GtkWidget *list = nullptr;
  std::unique_ptr<AlbumCoverFetcher> fetcher;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
};

void ClearList(GtkWidget *list) {
  if (!list) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(list)) {
    gtk_list_box_remove(GTK_LIST_BOX(list), child);
  }
}

void AddResult(SearcherState *state, const CoverProviderSearchResult &result) {
  if (!state || !state->list) {
    return;
  }
  GtkWidget *row = adw_action_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), AlbumCoverFetcherSearch::ResultLabel(result).c_str());
  adw_action_row_set_subtitle(ADW_ACTION_ROW(row), AlbumCoverFetcherSearch::ResultSubtitle(result).c_str());
  if (!result.image_data.empty()) {
    GtkWidget *thumb = gtk_image_new();
    SetImageFromBytes(thumb, std::vector<unsigned char>(result.image_data.begin(), result.image_data.end()), 48);
    adw_action_row_add_prefix(ADW_ACTION_ROW(row), thumb);
  }
  GtkWidget *apply = gtk_button_new_with_label(Translations::CStr("Save"));
  gtk_widget_add_css_class(apply, "suggested-action");
  auto *hit = new CoverProviderSearchResult(result);
  g_object_set_data_full(G_OBJECT(apply), "result", hit, [](gpointer p) { delete static_cast<CoverProviderSearchResult *>(p); });
  g_signal_connect(apply, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *self = static_cast<SearcherState *>(data);
                     auto *result = static_cast<CoverProviderSearchResult *>(g_object_get_data(G_OBJECT(button), "result"));
                     if (!self || !self->app || !result) {
                       return;
                     }
                     const std::string image = !result->image_data.empty() ? result->image_data : result->image_url;
                     if (ApplyCover(self->app, &self->song, image)) {
                       gtk_button_set_label(button, Translations::CStr("Saved"));
                     } else {
                       gtk_button_set_label(button, Translations::CStr("Failed"));
                     }
                   })),
                   state);
  adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
  gtk_list_box_append(GTK_LIST_BOX(state->list), row);
}

void StartSearch(SearcherState *state) {
  if (!state || !state->fetcher || !state->app) {
    return;
  }
  const std::string artist = gtk_editable_get_text(GTK_EDITABLE(state->artist));
  const std::string album = gtk_editable_get_text(GTK_EDITABLE(state->album));
  const std::string title = gtk_editable_get_text(GTK_EDITABLE(state->title));
  state->song.set_artist(artist);
  state->song.set_albumartist(artist);
  state->song.set_album(album);
  state->song.set_title(title);
  ClearList(state->list);
  gtk_label_set_text(GTK_LABEL(state->status), AlbumCoverFetcherSearch::StatusSearching(album).c_str());
  state->fetcher->SearchForCovers(artist, album, title);
}

}  // namespace

void AlbumCoverSearcher::Show(GtkWindow *parent, Application *app) {
  Song song = SongForDialog(app);
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Cover search"));
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 620);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  auto *state = new SearcherState();
  state->app = app;
  state->song = song;
  state->fetcher = std::make_unique<AlbumCoverFetcher>(app->cover_providers(), app->network());

  auto add_entry = [&](const char *label, const std::string &value) {
    AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(label));
    gtk_editable_set_text(GTK_EDITABLE(row), value.c_str());
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(row));
    return GTK_WIDGET(row);
  };
  state->artist = add_entry("Artist", song.EffectiveAlbumartist());
  state->album = add_entry("Album", song.album());
  state->title = add_entry("Title", song.title());

  GtkWidget *search = gtk_button_new_with_label(Translations::CStr("Search"));
  gtk_widget_add_css_class(search, "suggested-action");
  gtk_box_append(GTK_BOX(box), search);

  state->status = gtk_label_new(AlbumCoverFetcherSearch::StatusSearching(song.album()).c_str());
  gtk_label_set_wrap(GTK_LABEL(state->status), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->status), 0);
  gtk_box_append(GTK_BOX(box), state->status);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  state->list = gtk_list_box_new();
  gtk_widget_add_css_class(state->list, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), state->list);
  gtk_box_append(GTK_BOX(box), scroll);

  g_object_set_data_full(G_OBJECT(dialog), "searcher-state", state, [](gpointer p) {
    auto *self = static_cast<SearcherState *>(p);
    if (self->alive) {
      *self->alive = false;
    }
    if (self->fetcher) {
      self->fetcher->Clear();
    }
    delete self;
  });

  const auto alive = state->alive;
  state->fetcher->SearchFinished.Connect([state, alive](uint64_t, const CoverProviderSearchResults &results, const CoverSearchStatistics &) {
    if (!alive || !*alive || !state) {
      return;
    }
    ClearList(state->list);
    for (const CoverProviderSearchResult &result : results) {
      AddResult(state, result);
    }
    gtk_label_set_text(GTK_LABEL(state->status), AlbumCoverFetcherSearch::StatusFound(static_cast<int>(results.size())).c_str());
  });

  g_signal_connect(search, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { StartSearch(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->album, "apply", G_CALLBACK(+[](AdwEntryRow *, gpointer data) { StartSearch(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->artist, "apply", G_CALLBACK(+[](AdwEntryRow *, gpointer data) { StartSearch(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->title, "apply", G_CALLBACK(+[](AdwEntryRow *, gpointer data) { StartSearch(static_cast<SearcherState *>(data)); }), state);

  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  StartSearch(state);
}
