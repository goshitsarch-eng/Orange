#include "collection/collectionview.h"

#include "collection/collectionbehaviour.h"
#include "collection/collectionitemdelegate.h"
#include "translations/translations.h"
#include "utilities/strutils.h"

#include <gdk/gdkkeysyms.h>

CollectionView::CollectionView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_hexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     static_cast<CollectionView *>(data)->ActivateRow(row);
                   }),
                   this);

  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<CollectionView *>(data);
                     GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y));
                     if (row && !gtk_list_box_row_is_selected(row)) {
                       gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                       gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                     }
                     if (self->menu_) {
                       self->menu_(x, y);
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);

  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  g_signal_connect(keys, "key-pressed", G_CALLBACK(+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<CollectionView *>(data)->OnKeyPressed(keyval);
                   }),
                   this);
}

CollectionView::~CollectionView() { ResetTypeAhead(); }

void CollectionView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void CollectionView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void CollectionView::SetFilterString(const std::string &filter) {
  filter_.SetFilterString(filter);
  Rebuild();
}

void CollectionView::SetModelSongs(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping,
                                   bool skip_artist_articles, bool skip_album_articles) {
  model_.Reset(songs, grouping, separate_albums_by_grouping, skip_artist_articles, skip_album_articles);
  Rebuild();
}

void CollectionView::ActivateRow(GtkListBoxRow *row) {
  auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
  if (item && activate_) {
    activate_(model_.SongsFromItem(item));
  }
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
  ResetTypeAhead();
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

std::vector<const CollectionItem *> CollectionView::SelectedItems() const {
  std::vector<const CollectionItem *> items;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
        if (item) {
          static_cast<std::vector<const CollectionItem *> *>(data)->push_back(item);
        }
      },
      &items);
  return items;
}

const CollectionItem *CollectionView::SelectedItem() const {
  const auto items = SelectedItems();
  return items.empty() ? nullptr : items.front();
}

SongList CollectionView::SelectedSongs() const {
  SongList songs;
  for (const CollectionItem *item : SelectedItems()) {
    const SongList from_item = model_.SongsFromItem(item);
    songs.insert(songs.end(), from_item.begin(), from_item.end());
  }
  return CollectionBehaviour::UniqueByUrl(songs);
}

void CollectionView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_id_) {
    g_source_remove(typeahead_timeout_id_);
    typeahead_timeout_id_ = 0;
  }
}

void CollectionView::TypeAhead(gunichar ch) {
  gchar utf8[8] = {};
  const gint len = g_unichar_to_utf8(ch, utf8);
  typeahead_.append(utf8, static_cast<size_t>(len));
  if (typeahead_timeout_id_) {
    g_source_remove(typeahead_timeout_id_);
  }
  typeahead_timeout_id_ = g_timeout_add(1000, [](gpointer data) -> gboolean {
    auto *self = static_cast<CollectionView *>(data);
    self->typeahead_timeout_id_ = 0;
    self->typeahead_.clear();
    return G_SOURCE_REMOVE;
  }, this);

  const std::string needle = StrUtils::ToLower(typeahead_);
  for (GtkWidget *child = gtk_widget_get_first_child(list_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(child), "item"));
    if (!item) {
      continue;
    }
    if (StrUtils::StartsWith(StrUtils::ToLower(CollectionItemDelegate::PrimaryText(item)), needle)) {
      gtk_list_box_unselect_all(GTK_LIST_BOX(list_));
      gtk_list_box_select_row(GTK_LIST_BOX(list_), GTK_LIST_BOX_ROW(child));
      gtk_widget_grab_focus(child);
      return;
    }
  }
}

gboolean CollectionView::OnKeyPressed(guint keyval) {
  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
    if (GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_))) {
      ActivateRow(row);
      return TRUE;
    }
  }
  if (keyval == GDK_KEY_Escape) {
    ResetTypeAhead();
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch && g_unichar_isprint(ch)) {
    TypeAhead(ch);
    return TRUE;
  }
  return FALSE;
}
