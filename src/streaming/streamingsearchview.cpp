#include "streaming/streamingsearchview.h"

#include "streaming/streamingdrag.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "translations/translations.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"

StreamingSearchView::StreamingSearchView(StreamingService *service) : service_(service) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  search_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_entry_), Translations::CStr("Search"));
  gtk_widget_set_margin_start(search_entry_, 8);
  gtk_widget_set_margin_end(search_entry_, 8);
  gtk_widget_set_margin_top(search_entry_, 6);
  gtk_widget_set_margin_bottom(search_entry_, 4);
  GtkWidget *types = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(types, 8);
  gtk_widget_set_margin_end(types, 8);
  gtk_widget_set_margin_bottom(types, 4);
  type_artists_ = gtk_toggle_button_new_with_label("Artists");
  type_albums_ = gtk_toggle_button_new_with_label("Albums");
  type_songs_ = gtk_toggle_button_new_with_label("Songs");
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(type_albums_), GTK_TOGGLE_BUTTON(type_artists_));
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(type_songs_), GTK_TOGGLE_BUTTON(type_artists_));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(type_songs_), TRUE);
  gtk_box_append(GTK_BOX(types), type_artists_);
  gtk_box_append(GTK_BOX(types), type_albums_);
  gtk_box_append(GTK_BOX(types), type_songs_);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  auto search_now = +[](GtkWidget *, gpointer data) {
    auto *self = static_cast<StreamingSearchView *>(data);
    const char *text = gtk_editable_get_text(GTK_EDITABLE(self->search_entry_));
    self->Search(text ? text : "");
  };
  g_signal_connect(search_entry_, "activate", G_CALLBACK(search_now), this);
  g_signal_connect(search_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     const std::string query = text ? text : "";
                     if (query.size() >= 2) {
                       self->Search(query);
                     }
                   }),
                   this);
  g_signal_connect(type_artists_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(type_albums_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(type_songs_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song && self->activate_) {
                       self->activate_(*song);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     if (self->menu_) {
                       self->menu_(self->SelectedSongs());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), search_entry_);
  gtk_box_append(GTK_BOX(widget_), types);
  gtk_box_append(GTK_BOX(widget_), scroll);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<StreamingSearchView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

StreamingSearchView::~StreamingSearchView() { ResetTypeAhead(); }

void StreamingSearchView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void StreamingSearchView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void StreamingSearchView::Search(const std::string &query) {
  if (!service_ || query.empty()) {
    return;
  }
  StreamingService::SearchType type = StreamingService::SearchType::Songs;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_artists_))) {
    type = StreamingService::SearchType::Artists;
  } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_albums_))) {
    type = StreamingService::SearchType::Albums;
  }
  model_.SetSearchType(type);
  service_->Search(query, type, [this](const SongList &songs) {
    model_.SetSongs(songs);
    Rebuild();
  });
}

void StreamingSearchView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList visible = sort_model_.Visible();
  if (visible.empty()) {
    gtk_list_box_append(GTK_LIST_BOX(list_), gtk_label_new(Translations::CStr("No results")));
    return;
  }
  for (const Song &song : visible) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);
    GtkWidget *primary = gtk_label_new(StreamingSearchItemDelegate::PrimaryText(song).c_str());
    gtk_widget_set_halign(primary, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), primary);
    const std::string secondary = StreamingSearchItemDelegate::SecondaryText(song);
    if (!secondary.empty()) {
      GtkWidget *sub = gtk_label_new(secondary.c_str());
      gtk_widget_add_css_class(sub, "dim-label");
      gtk_widget_set_halign(sub, GTK_ALIGN_START);
      gtk_box_append(GTK_BOX(box), sub);
    }
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    g_object_set_data_full(G_OBJECT(row), "row-data", new Song(song), [](gpointer p) { delete static_cast<Song *>(p); });
    SetupRowDrag(row, song);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void StreamingSearchView::SetupRowDrag(GtkWidget *row, const Song &song) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  g_object_set_data_full(G_OBJECT(src), "row-data", new Song(song), [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     auto *dragged = static_cast<Song *>(g_object_get_data(G_OBJECT(s), "row-data"));
                     SongList songs = dragged ? SongList{*dragged} : SongList{};
                     for (const Song &selected : self->SelectedSongs()) {
                       if (dragged && selected.url() == dragged->url()) {
                         songs = self->SelectedSongs();
                         break;
                       }
                     }
                     const std::string payload = StreamingDrag::DragPayload(songs);
                     if (payload.empty()) {
                       return nullptr;
                     }
                     GValue v = G_VALUE_INIT;
                     g_value_init(&v, G_TYPE_STRING);
                     g_value_set_string(&v, payload.c_str());
                     GdkContentProvider *provider = gdk_content_provider_new_for_value(&v);
                     g_value_unset(&v);
                     return provider;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(src));
}

void StreamingSearchView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean StreamingSearchView::OnKeyPressed(guint keyval) {
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if (action == ListBoxKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::MoveUp || action == ListBoxKeyboard::Action::MoveDown || action == ListBoxKeyboard::Action::Home ||
      action == ListBoxKeyboard::Action::End) {
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(ListBoxKeyboardGtk::SelectedIndex(list_),
                                                                      ListBoxKeyboardGtk::Count(list_), action));
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Escape) {
    ResetTypeAhead();
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch && g_unichar_isprint(ch)) {
    gchar utf8[8] = {};
    typeahead_.append(utf8, static_cast<size_t>(g_unichar_to_utf8(ch, utf8)));
    if (typeahead_timeout_) {
      g_source_remove(typeahead_timeout_);
    }
    typeahead_timeout_ = g_timeout_add(1000, [](gpointer data) -> gboolean {
      auto *self = static_cast<StreamingSearchView *>(data);
      self->typeahead_timeout_ = 0;
      self->typeahead_.clear();
      return G_SOURCE_REMOVE;
    }, this);
    const int index = ListBoxKeyboard::FirstPrefixIndex(ListBoxKeyboardGtk::Labels(list_), typeahead_);
    if (index >= 0) {
      ListBoxKeyboardGtk::SelectIndex(list_, index);
    }
    return TRUE;
  }
  return FALSE;
}

SongList StreamingSearchView::SelectedSongs() const {
  SongList songs;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
        if (song) {
          static_cast<SongList *>(data)->push_back(*song);
        }
      },
      &songs);
  return songs;
}
