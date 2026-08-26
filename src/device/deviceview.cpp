#include "device/deviceview.h"

#include "collection/collectiongrouping.h"
#include "collection/collectionitemdelegate.h"
#include "collection/collectiontree.h"
#include "device/devicekeyboard.h"
#include "device/devicedrag.h"
#include "device/deviceviewlook.h"
#include "translations/translations.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"
#include "widgets/listboxtreepressgtk.h"

#include <string>

DeviceView::DeviceView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  ListBoxTreePressGtk::Attach(list_, this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<DeviceView *>(data);
                     const char *kind = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "row-kind"));
                     if (kind && std::string(kind) == "back" && self->back_cb_) {
                       self->back_cb_();
                       return;
                     }
                     if (kind && std::string(kind) == "add-all" && self->add_all_cb_) {
                       self->add_all_cb_();
                       return;
                     }
                     auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
                     if (item) {
                       const SongList songs = CollectionTree::SongsFromItem(item);
                       if (self->songs_cb_ && !songs.empty()) {
                         self->songs_cb_(songs);
                       } else if (self->song_cb_ && !songs.empty()) {
                         self->song_cb_(songs.front());
                       }
                       return;
                     }
                     if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "song"))) {
                       if (self->song_cb_) {
                         self->song_cb_(*song);
                       }
                       return;
                     }
                     if (auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(row), "device"))) {
                       if (self->device_cb_) {
                         self->device_cb_(device->unique_id);
                       }
                     }
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<DeviceView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

DeviceView::~DeviceView() { ResetTypeAhead(); }

void DeviceView::HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state) {
  const CollectionTreeClick::Action action = CollectionTreeClick::FromPress(button, n_press, state);
  GtkListBoxRow *row = ListBoxTreePressGtk::RowAtY(list_, y);
  if (action == CollectionTreeClick::Action::Enqueue) {
    if (row && CollectionTreeClick::SelectRowBeforeEnqueue(gtk_list_box_row_is_selected(row))) {
      ListBoxTreePressGtk::SelectRowIfNeeded(list_, row);
    }
    if (enqueue_) {
      enqueue_(SelectedSongs());
    }
    return;
  }
  if (action != CollectionTreeClick::Action::ToggleExpand || !row || ListBoxTreePressGtk::OnExpandControl(list_, x, y)) {
    return;
  }
  auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
  if (CollectionTreeClick::ShouldToggleFromRowClick(false, CollectionTree::IsExpandable(item))) {
    ToggleExpanded(item);
  }
}

void DeviceView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean DeviceView::OnKeyPressed(guint keyval) {
  const DeviceKeyboard::Action action = DeviceKeyboard::FromKey(keyval);
  if (action == DeviceKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == DeviceKeyboard::Action::Back && back_cb_) {
    back_cb_();
    return TRUE;
  }
  if (action == DeviceKeyboard::Action::MoveUp || action == DeviceKeyboard::Action::MoveDown || action == DeviceKeyboard::Action::Home ||
      action == DeviceKeyboard::Action::End) {
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(ListBoxKeyboardGtk::SelectedIndex(list_),
                                                                      ListBoxKeyboardGtk::Count(list_), DeviceKeyboard::MoveAction(action)));
    return TRUE;
  }
  if (action == DeviceKeyboard::Action::Escape) {
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
      auto *self = static_cast<DeviceView *>(data);
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

void DeviceView::AttachMenu(GtkWidget *row) {
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<DeviceView *>(data);
                     GtkWidget *row = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(click));
                     if (GTK_IS_LIST_BOX_ROW(row) && !gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
                       gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                       gtk_list_box_select_row(GTK_LIST_BOX(self->list_), GTK_LIST_BOX_ROW(row));
                     }
                     if (auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(row), "device"))) {
                       if (self->device_menu_cb_) {
                         self->device_menu_cb_(*device);
                       }
                     } else if (auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"))) {
                       const SongList songs = CollectionTree::SongsFromItem(item);
                       if (self->song_menu_cb_ && !songs.empty()) {
                         self->song_menu_cb_(songs.front());
                       }
                     } else if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "song"))) {
                       if (self->song_menu_cb_) {
                         self->song_menu_cb_(*song);
                       }
                     }
                   }),
                   this);
}

void DeviceView::Clear() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
}

void DeviceView::ShowDevices(const std::vector<ConnectedDevice> &devices) {
  Clear();
  if (devices.empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), gtk_label_new(Translations::CStr("No devices found")));
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    return;
  }
  for (const ConnectedDevice &device : devices) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(row_box, 12);
    gtk_widget_set_margin_end(row_box, 12);
    gtk_widget_set_margin_top(row_box, 8);
    gtk_widget_set_margin_bottom(row_box, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(DeviceViewLook::IconName(device));
    gtk_image_set_pixel_size(GTK_IMAGE(icon), DeviceViewLook::kIconSize);
    gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
    GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(text, TRUE);
    GtkWidget *primary = gtk_label_new(device.friendly_name.c_str());
    gtk_widget_set_halign(primary, GTK_ALIGN_START);
    gtk_widget_add_css_class(primary, "heading");
    GtkWidget *status = gtk_label_new(DeviceViewLook::StatusText(device, -1, device.remembered).c_str());
    gtk_widget_add_css_class(status, "dim-label");
    gtk_widget_set_halign(status, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(status), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(text), primary);
    gtk_box_append(GTK_BOX(text), status);
    gtk_box_append(GTK_BOX(row_box), icon);
    gtk_box_append(GTK_BOX(row_box), text);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    auto *copy = new ConnectedDevice(device);
    g_object_set_data_full(G_OBJECT(row), "device", copy, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
    AttachMenu(row);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void DeviceView::ToggleExpanded(const CollectionItem *item) {
  if (CollectionTree::Toggle(&expanded_, item) || CollectionTree::IsExpandable(item)) {
    RebuildSongs();
  }
}

void DeviceView::AppendItem(const CollectionItem *item, int depth) {
  if (!item) {
    return;
  }
  const bool expandable = CollectionTree::IsExpandable(item);
  const bool expanded = CollectionTree::ShowChildren(item, false, expanded_);
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(row_box, 8 + depth * 12);
  gtk_widget_set_margin_end(row_box, 8);
  gtk_widget_set_margin_top(row_box, 4);
  gtk_widget_set_margin_bottom(row_box, 4);
  if (expandable) {
    GtkWidget *toggle = gtk_button_new_from_icon_name(expanded ? "pan-down-symbolic" : "pan-end-symbolic");
    gtk_widget_add_css_class(toggle, "flat");
    gtk_widget_add_css_class(toggle, "circular");
    gtk_widget_set_tooltip_text(toggle, expanded ? Translations::CStr("Collapse") : Translations::CStr("Expand"));
    g_object_set_data(G_OBJECT(toggle), "item", const_cast<CollectionItem *>(item));
    g_signal_connect(toggle, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                       auto *self = static_cast<DeviceView *>(data);
                       self->ToggleExpanded(static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(button), "item")));
                     }),
                     this);
    gtk_box_append(GTK_BOX(row_box), toggle);
  }
  GtkWidget *icon = gtk_image_new_from_icon_name(DeviceViewLook::ItemIconName(item));
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
  gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(row_box), icon);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(box, TRUE);
  GtkWidget *primary = gtk_label_new(CollectionItemDelegate::PrimaryText(item).c_str());
  gtk_widget_set_halign(primary, GTK_ALIGN_START);
  if (expandable) {
    gtk_widget_add_css_class(primary, "heading");
  }
  gtk_box_append(GTK_BOX(box), primary);
  const std::string secondary = CollectionItemDelegate::SecondaryText(item);
  if (!secondary.empty()) {
    GtkWidget *sub = gtk_label_new(secondary.c_str());
    gtk_widget_add_css_class(sub, "dim-label");
    gtk_widget_set_halign(sub, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), sub);
  }
  gtk_box_append(GTK_BOX(row_box), box);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
  g_object_set_data(G_OBJECT(row), "item", const_cast<CollectionItem *>(item));
  const SongList songs = CollectionTree::SongsFromItem(item);
  if (!songs.empty()) {
    auto *copy = new Song(songs.front());
    g_object_set_data_full(G_OBJECT(row), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    SetupRowDrag(row, songs.front());
  }
  AttachMenu(row);
  gtk_list_box_append(GTK_LIST_BOX(list_), row);
  if (expanded) {
    for (const auto &child : item->children) {
      AppendItem(child.get(), depth + 1);
    }
  }
}

void DeviceView::ShowSongs(const SongList &songs) {
  songs_ = songs;
  RebuildSongs();
}

void DeviceView::RebuildSongs() {
  Clear();
  GtkWidget *back = gtk_list_box_row_new();
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(back), gtk_label_new(Translations::CStr("← Devices")));
  g_object_set_data(G_OBJECT(back), "row-kind", const_cast<char *>("back"));
  gtk_list_box_append(GTK_LIST_BOX(list_), back);
  GtkWidget *add_all = gtk_list_box_row_new();
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(add_all), gtk_label_new(Translations::CStr("Add all to playlist")));
  g_object_set_data(G_OBJECT(add_all), "row-kind", const_cast<char *>("add-all"));
  gtk_list_box_append(GTK_LIST_BOX(list_), add_all);
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  model_.Reset(songs_, grouping, false, false, false);
  if (songs_.empty()) {
    GtkWidget *empty = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(empty), gtk_label_new(Translations::CStr("No songs found on this device")));
    gtk_list_box_append(GTK_LIST_BOX(list_), empty);
    return;
  }
  if (model_.root()) {
    for (const auto &child : model_.root()->children) {
      AppendItem(child.get(), 0);
    }
  }
}

void DeviceView::SetupRowDrag(GtkWidget *row, const Song &song) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(src), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<DeviceView *>(data);
                     auto *dragged = static_cast<Song *>(g_object_get_data(G_OBJECT(s), "song"));
                     SongList songs = dragged ? SongList{*dragged} : SongList{};
                     for (const Song &selected : self->SelectedSongs()) {
                       if (dragged && selected.url() == dragged->url()) {
                         songs = self->SelectedSongs();
                         break;
                       }
                     }
                     const std::string payload = DeviceDrag::DragPayload(songs);
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

const ConnectedDevice *DeviceView::SelectedDevice() const {
  const ConnectedDevice *selected = nullptr;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        if (auto *device = static_cast<ConnectedDevice *>(g_object_get_data(G_OBJECT(row), "device"))) {
          *static_cast<const ConnectedDevice **>(data) = device;
        }
      },
      &selected);
  return selected;
}

SongList DeviceView::SelectedSongs() const {
  SongList songs;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
        const SongList more = CollectionTree::SongsFromItem(item);
        if (!more.empty()) {
          static_cast<SongList *>(data)->insert(static_cast<SongList *>(data)->end(), more.begin(), more.end());
          return;
        }
        if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "song"))) {
          static_cast<SongList *>(data)->push_back(*song);
        }
      },
      &songs);
  return songs;
}
