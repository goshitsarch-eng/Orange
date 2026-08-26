#include "collection/collectionview.h"

#include "collection/collectionitemdelegate.h"
#include "translations/translations.h"

CollectionView::CollectionView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_hexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<CollectionView *>(data);
                     auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
                     if (item && self->activate_) {
                       self->activate_(self->model_.SongsFromItem(item));
                     }
                   }),
                   this);
}

void CollectionView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void CollectionView::SetFilterString(const std::string &filter) {
  filter_.SetFilterString(filter);
  Rebuild();
}

void CollectionView::SetModelSongs(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
                                   bool skip_artist_articles, bool skip_album_articles) {
  model_.Reset(songs, grouping, separate_albums_by_grouping, skip_artist_articles, skip_album_articles);
  Rebuild();
}

void CollectionView::AppendItem(GtkWidget *parent, const CollectionItem *item, int depth) {
  if (!item || !filter_.AcceptsItem(item)) {
    return;
  }
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_start(box, 8 + depth * 12);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 4);
  gtk_widget_set_margin_bottom(box, 4);
  GtkWidget *primary = gtk_label_new(CollectionItemDelegate::PrimaryText(item).c_str());
  gtk_widget_set_halign(primary, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), primary);
  const std::string secondary = CollectionItemDelegate::SecondaryText(item);
  if (!secondary.empty()) {
    GtkWidget *sub = gtk_label_new(secondary.c_str());
    gtk_widget_add_css_class(sub, "dim-label");
    gtk_widget_set_halign(sub, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), sub);
  }
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
  g_object_set_data(G_OBJECT(row), "item", const_cast<CollectionItem *>(item));
  if (GTK_IS_LIST_BOX(parent)) {
    gtk_list_box_append(GTK_LIST_BOX(parent), row);
  }
  for (const auto &child : item->children) {
    AppendItem(parent, child.get(), depth + 1);
  }
}

void CollectionView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (!model_.root()) {
    return;
  }
  for (const auto &child : model_.root()->children) {
    AppendItem(list_, child.get(), 0);
  }
  if (!gtk_widget_get_first_child(list_)) {
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), gtk_label_new(Translations::CStr("Collection is empty")));
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
