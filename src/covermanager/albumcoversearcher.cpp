#include "covermanager/albumcoversearcher.h"

#include "covermanager/albumcoversearcherlabels.h"
#include "core/application.h"
#include "core/network.h"
#include "dialogs/dialogclosekeys.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"
#include "utilities/jsonutils.h"

#include <adwaita.h>

#include <functional>
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
  GtkWidget *grid = nullptr;
  GtkWidget *search = nullptr;
  GtkWidget *spinner = nullptr;
  std::unique_ptr<AlbumCoverFetcher> fetcher;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
  std::function<void(bool)> done;
  int search_gen = 0;
  uint64_t search_id = 0;
  bool searching = false;
};

void ClearGrid(GtkWidget *grid) {
  if (!grid) {
    return;
  }
  while (GtkWidget *child = gtk_widget_get_first_child(grid)) {
    gtk_flow_box_remove(GTK_FLOW_BOX(grid), child);
  }
}

void SaveResult(SearcherState *state, CoverProviderSearchResult *result, GtkWidget *status_label) {
  if (!state || !state->app || !result) {
    return;
  }
  const std::string image = !result->image_data.empty() ? result->image_data : result->image_url;
  if (ApplyCover(state->app, &state->song, image)) {
    if (status_label) {
      gtk_label_set_text(GTK_LABEL(status_label), Translations::CStr("Saved"));
    }
    if (state->done) {
      state->done(true);
    }
  } else if (status_label) {
    gtk_label_set_text(GTK_LABEL(status_label), Translations::CStr("Failed"));
    if (state->done) {
      state->done(false);
    }
  } else if (state->done) {
    state->done(false);
  }
}

void LoadThumb(SearcherState *state, GtkWidget *image, GtkWidget *size_label, CoverProviderSearchResult *result, int gen) {
  if (!state || !image || !result) {
    return;
  }
  if (!result->image_data.empty()) {
    SetImageFromBytes(image, std::vector<unsigned char>(result->image_data.begin(), result->image_data.end()), AlbumCoverSearcher::kIconSize);
    return;
  }
  if (!state->app || !state->app->network() || !AlbumCoverFetcherSearch::IsHttpUrl(result->image_url)) {
    return;
  }
  const auto alive = state->alive;
  state->app->network()->Get(result->image_url, [state, alive, gen, image, size_label, result](const NetworkAccessManager::Response &response) {
    if (!alive || !*alive || !state || gen != state->search_gen || !result) {
      return;
    }
    if (!response.ok() || !JsonUtils::LooksLikeImage(response.body)) {
      return;
    }
    result->image_data = response.body;
    SetImageFromBytes(image, std::vector<unsigned char>(response.body.begin(), response.body.end()), AlbumCoverSearcher::kIconSize);
    GdkPaintable *paintable = gtk_image_get_paintable(GTK_IMAGE(image));
    if (GDK_IS_TEXTURE(paintable)) {
      result->image_width = gdk_texture_get_width(GDK_TEXTURE(paintable));
      result->image_height = gdk_texture_get_height(GDK_TEXTURE(paintable));
      if (size_label) {
        gtk_label_set_text(GTK_LABEL(size_label), AlbumCoverSearcher::CellSubtitle(*result).c_str());
      }
    }
  });
}

void AddResult(SearcherState *state, const CoverProviderSearchResult &result) {
  if (!state || !state->grid) {
    return;
  }
  GtkWidget *cell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_size_request(cell, AlbumCoverSearcher::kIconSize + 8, AlbumCoverSearcher::kIconSize + 36);
  GtkWidget *thumb = gtk_image_new_from_icon_name("image-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(thumb), AlbumCoverSearcher::kIconSize);
  gtk_widget_set_halign(thumb, GTK_ALIGN_CENTER);
  GtkWidget *caption = gtk_label_new(AlbumCoverSearcher::CellSubtitle(result).c_str());
  gtk_label_set_ellipsize(GTK_LABEL(caption), PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign(GTK_LABEL(caption), 0.5);
  gtk_widget_add_css_class(caption, "dim-label");
  gtk_box_append(GTK_BOX(cell), thumb);
  gtk_box_append(GTK_BOX(cell), caption);
  auto *hit = new CoverProviderSearchResult(result);
  g_object_set_data_full(G_OBJECT(cell), "result", hit, [](gpointer p) { delete static_cast<CoverProviderSearchResult *>(p); });
  gtk_flow_box_append(GTK_FLOW_BOX(state->grid), cell);
  LoadThumb(state, thumb, caption, hit, state->search_gen);
}

void ApplySearchUi(SearcherState *state) {
  if (!state) {
    return;
  }
  if (state->search) {
    gtk_button_set_label(GTK_BUTTON(state->search), Translations::CStr(AlbumCoverSearcher::SearchButtonLabel(state->searching)));
  }
  if (state->artist) {
    gtk_widget_set_sensitive(state->artist, AlbumCoverSearcher::FieldsEnabled(state->searching));
  }
  if (state->album) {
    gtk_widget_set_sensitive(state->album, AlbumCoverSearcher::FieldsEnabled(state->searching));
  }
  if (state->title) {
    gtk_widget_set_sensitive(state->title, AlbumCoverSearcher::FieldsEnabled(state->searching));
  }
  if (state->grid) {
    gtk_widget_set_sensitive(state->grid, AlbumCoverSearcher::GridEnabled(state->searching));
  }
  if (state->spinner) {
    gtk_widget_set_visible(state->spinner, AlbumCoverSearcher::BusyVisible(state->searching));
    if (state->searching) {
      gtk_spinner_start(GTK_SPINNER(state->spinner));
    } else {
      gtk_spinner_stop(GTK_SPINNER(state->spinner));
    }
  }
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
  ++state->search_gen;
  ClearGrid(state->grid);
  gtk_label_set_text(GTK_LABEL(state->status), AlbumCoverFetcherSearch::StatusSearching(album).c_str());
  state->searching = true;
  ApplySearchUi(state);
  state->search_id = state->fetcher->SearchForCovers(artist, album, title);
}

void AbortSearch(SearcherState *state) {
  if (!state) {
    return;
  }
  ++state->search_gen;
  state->search_id = 0;
  ClearGrid(state->grid);
  if (state->fetcher) {
    state->fetcher->Clear();
  }
  state->searching = false;
  if (state->status) {
    gtk_label_set_text(GTK_LABEL(state->status), "");
  }
  ApplySearchUi(state);
}

void OnSearchClicked(SearcherState *state) {
  if (!state) {
    return;
  }
  ClearGrid(state->grid);
  if (AlbumCoverSearcher::ShouldStartSearch(state->searching)) {
    StartSearch(state);
  } else {
    AbortSearch(state);
  }
}

}  // namespace

void AlbumCoverSearcher::Show(GtkWindow *parent, Application *app) { Show(parent, app, Song(), {}); }

void AlbumCoverSearcher::Show(GtkWindow *parent, Application *app, const Song &song, std::function<void(bool)> done) {
  const Song target = PreferredSong(song, SongForDialog(app));
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(AlbumCoverSearcherLabels::Title()));
  adw_dialog_set_content_width(dialog, 640);
  adw_dialog_set_content_height(dialog, 680);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  auto *state = new SearcherState();
  state->app = app;
  state->song = target;
  state->done = std::move(done);
  state->fetcher = std::make_unique<AlbumCoverFetcher>(app->cover_providers(), app->network());

  auto add_entry = [&](const char *label, const std::string &value) {
    AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), Translations::CStr(label));
    gtk_editable_set_text(GTK_EDITABLE(row), value.c_str());
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(row));
    return GTK_WIDGET(row);
  };
  state->artist = add_entry(AlbumCoverSearcherLabels::Artist(), target.EffectiveAlbumartist());
  state->album = add_entry(AlbumCoverSearcherLabels::Album(), target.album());
  state->title = add_entry("Title", target.title());

  GtkWidget *search_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  state->search = gtk_button_new_with_label(Translations::CStr(AlbumCoverSearcherLabels::Search()));
  gtk_widget_add_css_class(state->search, "suggested-action");
  state->spinner = gtk_spinner_new();
  gtk_widget_set_visible(state->spinner, FALSE);
  gtk_box_append(GTK_BOX(search_row), state->search);
  gtk_box_append(GTK_BOX(search_row), state->spinner);
  gtk_box_append(GTK_BOX(box), search_row);

  state->status = gtk_label_new(AlbumCoverFetcherSearch::StatusSearching(target.album()).c_str());
  gtk_label_set_wrap(GTK_LABEL(state->status), TRUE);
  gtk_label_set_xalign(GTK_LABEL(state->status), 0);
  gtk_box_append(GTK_BOX(box), state->status);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  state->grid = gtk_flow_box_new();
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(state->grid), kMinColumns);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(state->grid), kMaxColumns);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(state->grid), GTK_SELECTION_SINGLE);
  gtk_flow_box_set_activate_on_single_click(GTK_FLOW_BOX(state->grid), FALSE);
  gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(state->grid), TRUE);
  GtkEventController *grid_keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(grid_keys, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(state->grid, grid_keys);
  g_signal_connect(grid_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer) -> gboolean {
                     return AlbumCoverSearcher::ShouldIgnoreEnter(keyval) ? TRUE : FALSE;
                   })),
                   nullptr);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), state->grid);
  gtk_box_append(GTK_BOX(box), scroll);

  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save selected"));
  gtk_box_append(GTK_BOX(box), save);

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
  state->fetcher->SearchFinished.Connect([state, alive](uint64_t id, const CoverProviderSearchResults &results, const CoverSearchStatistics &) {
    if (!alive || !*alive || !state || id != state->search_id) {
      return;
    }
    state->searching = false;
    state->search_id = 0;
    ApplySearchUi(state);
    ClearGrid(state->grid);
    for (const CoverProviderSearchResult &result : results) {
      AddResult(state, result);
    }
    gtk_label_set_text(GTK_LABEL(state->status), AlbumCoverFetcherSearch::StatusFound(static_cast<int>(results.size())).c_str());
  });

  g_signal_connect(state->search, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { OnSearchClicked(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->album, "apply", G_CALLBACK(+[](AdwEntryRow *, gpointer data) { OnSearchClicked(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->artist, "apply", G_CALLBACK(+[](AdwEntryRow *, gpointer data) { OnSearchClicked(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->title, "apply", G_CALLBACK(+[](AdwEntryRow *, gpointer data) { OnSearchClicked(static_cast<SearcherState *>(data)); }), state);
  g_signal_connect(state->grid, "child-activated", G_CALLBACK((+[](GtkFlowBox *, GtkFlowBoxChild *child, gpointer data) {
                     auto *self = static_cast<SearcherState *>(data);
                     GtkWidget *cell = gtk_flow_box_child_get_child(child);
                     auto *result = cell ? static_cast<CoverProviderSearchResult *>(g_object_get_data(G_OBJECT(cell), "result")) : nullptr;
                     SaveResult(self, result, self ? self->status : nullptr);
                   })),
                   state);
  g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<SearcherState *>(data);
                     if (!self || !self->grid) {
                       return;
                     }
                     GList *selected = gtk_flow_box_get_selected_children(GTK_FLOW_BOX(self->grid));
                     if (!selected) {
                       return;
                     }
                     auto *child = GTK_FLOW_BOX_CHILD(selected->data);
                     GtkWidget *cell = gtk_flow_box_child_get_child(child);
                     auto *result = cell ? static_cast<CoverProviderSearchResult *>(g_object_get_data(G_OBJECT(cell), "result")) : nullptr;
                     SaveResult(self, result, self->status);
                     g_list_free(selected);
                   })),
                   state);

  adw_dialog_set_child(dialog, box);
  DialogCloseKeys::Attach(dialog);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  if (AlbumCoverSearcher::ShouldAutoSearch(target.EffectiveAlbumartist(), target.album())) {
    StartSearch(state);
  }
}
