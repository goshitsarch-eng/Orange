#include "smartplaylists/smartplaylistsearchpreview.h"

#include "playlist/playlistdelegates.h"
#include "smartplaylists/smartplaylistpreviewdisplay.h"
#include "smartplaylists/smartplaylistpreviewpolicy.h"
#include "translations/translations.h"

#include <memory>

namespace {

void ClearList(GtkWidget *list) {
  GtkWidget *child = gtk_widget_get_first_child(list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list), child);
    child = next;
  }
}

GtkWidget *ColumnLabel(const std::string &text, bool dim) {
  GtkWidget *label = gtk_label_new(text.c_str());
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_margin_start(label, 6);
  gtk_widget_set_margin_end(label, 6);
  if (dim) {
    gtk_widget_add_css_class(label, "dim-label");
  }
  return label;
}

GtkWidget *ColumnRow(const std::vector<std::string> &cells, bool header) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  for (const std::string &cell : cells) {
    gtk_box_append(GTK_BOX(box), ColumnLabel(cell, header));
  }
  return box;
}

struct PreviewSearchJob {
  SmartPlaylistSearchPreview *self = nullptr;
  std::shared_ptr<bool> alive;
  guint generation = 0;
  SmartPlaylistSearch search;
  SongList library;
  SongList matches;
};

gpointer PreviewSearchThread(gpointer data) {
  auto *job = static_cast<PreviewSearchJob *>(data);
  job->matches = job->search.Search(job->library);
  g_idle_add(+[](gpointer idle_data) -> gboolean {
    std::unique_ptr<PreviewSearchJob> finished(static_cast<PreviewSearchJob *>(idle_data));
    if (!finished->alive || !*finished->alive || !finished->self) {
      return G_SOURCE_REMOVE;
    }
    finished->self->OnSearchFinished(finished->generation, finished->search, std::move(finished->matches));
    return G_SOURCE_REMOVE;
  },
             job);
  return nullptr;
}

}  // namespace

SmartPlaylistSearchPreview::SmartPlaylistSearchPreview() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_vexpand(widget_, TRUE);
  label_ = gtk_label_new(SmartPlaylistPreviewDisplay::BusyText());
  gtk_widget_set_halign(label_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(widget_), label_);

  std::vector<std::string> titles;
  for (PlaylistColumn column : SmartPlaylistPreviewDisplay::Columns()) {
    titles.push_back(PlaylistDelegates::ColumnTitle(column));
  }
  header_ = ColumnRow(titles, true);
  gtk_box_append(GTK_BOX(widget_), header_);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_widget_set_hexpand(scroll, TRUE);
  gtk_widget_set_size_request(scroll, -1, 220);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  gtk_box_append(GTK_BOX(widget_), scroll);

  g_signal_connect(widget_, "map", G_CALLBACK((+[](GtkWidget *, gpointer data) {
                     static_cast<SmartPlaylistSearchPreview *>(data)->OnMapped();
                   })),
                   this);
}

SmartPlaylistSearchPreview::~SmartPlaylistSearchPreview() { *alive_ = false; }

bool SmartPlaylistSearchPreview::Hidden() const { return widget_ == nullptr || !gtk_widget_get_mapped(widget_); }

void SmartPlaylistSearchPreview::Update(const SmartPlaylistSearch &search, const SongList &songs) {
  library_ = songs;
  const bool same = have_last_search_ && SmartPlaylistPreviewPolicy::SameSearch(search, last_search_);
  switch (SmartPlaylistPreviewDisplay::DecideUpdate(same, busy_, Hidden())) {
    case SmartPlaylistPreviewDisplay::UpdateAction::Ignore:
      return;
    case SmartPlaylistPreviewDisplay::UpdateAction::Defer:
      pending_ = search;
      have_pending_ = true;
      return;
    case SmartPlaylistPreviewDisplay::UpdateAction::Run:
      RunSearch(search);
      return;
  }
}

void SmartPlaylistSearchPreview::OnMapped() {
  if (!SmartPlaylistPreviewDisplay::ShouldRunPendingOnShow(have_pending_, busy_)) {
    return;
  }
  const SmartPlaylistSearch next = pending_;
  have_pending_ = false;
  RunSearch(next);
}

void SmartPlaylistSearchPreview::RunSearch(const SmartPlaylistSearch &search) {
  ++generation_;
  busy_ = true;
  gtk_label_set_text(GTK_LABEL(label_), SmartPlaylistPreviewDisplay::BusyText());

  auto *job = new PreviewSearchJob();
  job->self = this;
  job->alive = alive_;
  job->generation = generation_;
  job->search = search;
  job->library = library_;
  g_thread_unref(g_thread_new("smart-preview", PreviewSearchThread, job));
}

void SmartPlaylistSearchPreview::OnSearchFinished(guint generation, const SmartPlaylistSearch &search, SongList matches) {
  if (generation != generation_) {
    return;
  }
  last_search_ = search;
  have_last_search_ = true;
  busy_ = false;

  if (SmartPlaylistPreviewDisplay::ShouldDiscardForPending(have_pending_, SmartPlaylistPreviewPolicy::SameSearch(pending_, search))) {
    const SmartPlaylistSearch next = pending_;
    have_pending_ = false;
    RunSearch(next);
    return;
  }
  have_pending_ = false;
  ApplyResults(matches);
  if (finished_) {
    finished_(match_count_);
  }
}

void SmartPlaylistSearchPreview::ApplyResults(const SongList &matches) {
  ClearList(list_);
  match_count_ = static_cast<int>(matches.size());
  const SongList shown = SmartPlaylistPreviewDisplay::SliceForDisplay(matches);
  const std::string count = SmartPlaylistPreviewDisplay::ApplyCountTemplate(
      Translations::Tr(SmartPlaylistPreviewDisplay::CountTemplate(static_cast<int>(shown.size()) < match_count_)), match_count_,
      static_cast<int>(shown.size()));
  gtk_label_set_text(GTK_LABEL(label_), count.c_str());
  for (const Song &song : shown) {
    std::vector<std::string> cells;
    for (PlaylistColumn column : SmartPlaylistPreviewDisplay::Columns()) {
      cells.push_back(SmartPlaylistPreviewDisplay::CellText(song, column));
    }
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), ColumnRow(cells, false));
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
