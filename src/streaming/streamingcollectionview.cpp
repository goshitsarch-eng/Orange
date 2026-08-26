#include "streaming/streamingcollectionview.h"

#include "collection/collectionfiltermenu.h"
#include "collection/groupbydialog.h"
#include "streaming/streamingdrag.h"
#include "streaming/streamingsearchgroup.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "translations/translations.h"
#include "utilities/strutils.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"

StreamingCollectionView::StreamingCollectionView(const std::string &title) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(header, 8);
  gtk_widget_set_margin_end(header, 8);
  gtk_widget_set_margin_top(header, 6);
  gtk_widget_set_margin_bottom(header, 4);
  GtkWidget *label = gtk_label_new(title.c_str());
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  back_ = gtk_button_new_from_icon_name("go-previous-symbolic");
  gtk_widget_set_tooltip_text(back_, "Back");
  gtk_widget_set_visible(back_, FALSE);
  g_signal_connect(back_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<StreamingCollectionView *>(data)->PopBrowse(); }),
                   this);
  GtkWidget *refresh = gtk_button_new_from_icon_name("view-refresh-symbolic");
  gtk_widget_set_tooltip_text(refresh, "Refresh");
  g_signal_connect(refresh, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     if (self->refresh_) {
                       self->refresh_();
                     }
                   }),
                   this);
  grouping_ = CollectionGrouping::LoadCurrent();
  group_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(group_button_), "view-list-symbolic");
  gtk_widget_set_tooltip_text(group_button_, Translations::CStr("Group by"));
  BuildGroupMenu();
  gtk_box_append(GTK_BOX(header), back_);
  gtk_box_append(GTK_BOX(header), label);
  gtk_box_append(GTK_BOX(header), group_button_);
  gtk_box_append(GTK_BOX(header), refresh);
  filter_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(filter_entry_), "Filter");
  gtk_widget_set_margin_start(filter_entry_, 8);
  gtk_widget_set_margin_end(filter_entry_, 8);
  g_signal_connect(filter_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->SetFilter(text ? text : "");
                   }),
                   this);
  status_label_ = gtk_label_new("");
  gtk_widget_set_margin_start(status_label_, 8);
  gtk_widget_set_halign(status_label_, GTK_ALIGN_START);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song) {
                       self->ActivateSong(*song);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     if (self->menu_) {
                       self->menu_(self->SelectedSongs());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), header);
  gtk_box_append(GTK_BOX(widget_), filter_entry_);
  gtk_box_append(GTK_BOX(widget_), status_label_);
  gtk_box_append(GTK_BOX(widget_), scroll);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<StreamingCollectionView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

StreamingCollectionView::~StreamingCollectionView() { ResetTypeAhead(); }

void StreamingCollectionView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void StreamingCollectionView::SetRefreshCallback(RefreshCallback callback) { refresh_ = std::move(callback); }

void StreamingCollectionView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void StreamingCollectionView::SetGrouping(const CollectionGrouping::Grouping &grouping) {
  if (grouping_ == grouping) {
    return;
  }
  grouping_ = grouping;
  Rebuild();
}

void StreamingCollectionView::ApplyGrouping(const CollectionGrouping::Grouping &grouping) {
  grouping_ = grouping;
  CollectionGrouping::SaveCurrent(grouping);
  if (grouping_changed_) {
    grouping_changed_(grouping_);
  }
  Rebuild();
}

void StreamingCollectionView::SetFilter(const std::string &filter) {
  filter_ = filter;
  Rebuild();
}

void StreamingCollectionView::SetStatus(const std::string &status) { gtk_label_set_text(GTK_LABEL(status_label_), status.c_str()); }

void StreamingCollectionView::SetSongs(const SongList &songs) {
  stack_.clear();
  songs_ = songs;
  Rebuild();
  UpdateBack();
}

void StreamingCollectionView::PushSongs(const SongList &songs) {
  stack_.push_back({songs_, status_label_ ? gtk_label_get_text(GTK_LABEL(status_label_)) : ""});
  songs_ = songs;
  Rebuild();
  UpdateBack();
}

void StreamingCollectionView::PopBrowse() {
  if (stack_.empty()) {
    return;
  }
  songs_ = stack_.back().songs;
  const std::string status = stack_.back().status;
  stack_.pop_back();
  Rebuild();
  if (status_label_) {
    gtk_label_set_text(GTK_LABEL(status_label_), status.c_str());
  }
  UpdateBack();
}

void StreamingCollectionView::UpdateBack() {
  if (back_) {
    gtk_widget_set_visible(back_, !stack_.empty());
  }
}

void StreamingCollectionView::ActivateSong(const Song &song) {
  if (activate_) {
    activate_(song);
  }
}

SongList StreamingCollectionView::Visible() const {
  if (filter_.empty()) {
    return songs_;
  }
  SongList visible;
  for (const Song &song : songs_) {
    if (StrUtils::ContainsInsensitive(song.PrettyTitleWithArtist(), filter_) || StrUtils::ContainsInsensitive(song.album(), filter_)) {
      visible.push_back(song);
    }
  }
  return visible;
}

void StreamingCollectionView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList visible = Visible();
  if (visible.empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), gtk_label_new(songs_.empty() ? "No items. Refresh to load." : "No matches"));
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    SetStatus(songs_.empty() ? "0 items" : "0 shown");
    return;
  }
  const std::vector<StreamingSearchGroup::Row> rows =
      StreamingSearchGroup::RowsFor(visible, grouping_, CollectionGrouping::SeparateAlbumsByGrouping());
  for (const StreamingSearchGroup::Row &entry : rows) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(box, StreamingSearchGroup::IndentPixels(entry.indent));
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);
    if (entry.header) {
      GtkWidget *label = gtk_label_new(entry.label.c_str());
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_add_css_class(label, "heading");
      gtk_box_append(GTK_BOX(box), label);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
      gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
      gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);
      gtk_list_box_append(GTK_LIST_BOX(list_), row);
      continue;
    }
    GtkWidget *primary = gtk_label_new(StreamingSearchItemDelegate::PrimaryText(entry.song).c_str());
    gtk_widget_set_halign(primary, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), primary);
    const std::string secondary = StreamingSearchItemDelegate::SecondaryText(entry.song);
    if (!secondary.empty()) {
      GtkWidget *sub = gtk_label_new(secondary.c_str());
      gtk_widget_add_css_class(sub, "dim-label");
      gtk_widget_set_halign(sub, GTK_ALIGN_START);
      gtk_box_append(GTK_BOX(box), sub);
    }
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    auto *copy = new Song(entry.song);
    g_object_set_data_full(G_OBJECT(row), "row-data", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    SetupRowDrag(row, entry.song);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
  SetStatus(std::to_string(StreamingSearchGroup::SongCount(rows)) + " items");
}

void StreamingCollectionView::BuildGroupMenu() {
  if (!group_button_) {
    return;
  }
  GMenu *menu = g_menu_new();
  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  for (size_t i = 0; i < presets.size(); ++i) {
    if (presets[i].advanced) {
      continue;
    }
    char action[64];
    g_snprintf(action, sizeof(action), "streamcoll.preset(%d)", static_cast<int>(i));
    g_menu_append(menu, Translations::CStr(presets[i].label), action);
  }
  g_menu_append(menu, Translations::CStr("Advanced grouping…"), "streamcoll.advanced");
  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *preset = g_simple_action_new("preset", G_VARIANT_TYPE_INT32);
  g_signal_connect(preset, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     const std::vector<CollectionFilterMenu::Preset> items = CollectionFilterMenu::BuiltinPresets();
                     const int index = g_variant_get_int32(param);
                     if (index < 0 || static_cast<size_t>(index) >= items.size() || items[static_cast<size_t>(index)].advanced) {
                       return;
                     }
                     self->ApplyGrouping(items[static_cast<size_t>(index)].grouping);
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(preset));
  GSimpleAction *advanced = g_simple_action_new("advanced", nullptr);
  g_signal_connect(advanced, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     GtkRoot *root = gtk_widget_get_root(self->widget_);
                     GtkWindow *parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
                     GroupByDialog::Show(parent, self->grouping_, [self](const CollectionGrouping::Grouping &grouping) {
                       self->ApplyGrouping(grouping);
                     });
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(advanced));
  gtk_widget_insert_action_group(group_button_, "streamcoll", G_ACTION_GROUP(group));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(group_button_), G_MENU_MODEL(menu));
  g_object_unref(group);
  g_object_unref(menu);
}

void StreamingCollectionView::SetupRowDrag(GtkWidget *row, const Song &song) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(src), "row-data", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<StreamingCollectionView *>(data);
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

SongList StreamingCollectionView::SelectedSongs() const {
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

void StreamingCollectionView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean StreamingCollectionView::OnKeyPressed(guint keyval) {
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if ((action == ListBoxKeyboard::Action::Backspace || action == ListBoxKeyboard::Action::Escape) && CanGoBack()) {
    PopBrowse();
    return TRUE;
  }
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
      auto *self = static_cast<StreamingCollectionView *>(data);
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
