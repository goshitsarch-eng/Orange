#include "device/deviceview.h"

#include "device/devicekeyboard.h"
#include "device/devicedrag.h"
#include "translations/translations.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"

#include <string>

DeviceView::DeviceView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
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
    GtkWidget *label = gtk_label_new((device.friendly_name + " · " + device.backend).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    auto *copy = new ConnectedDevice(device);
    g_object_set_data_full(G_OBJECT(row), "device", copy, [](gpointer p) { delete static_cast<ConnectedDevice *>(p); });
    AttachMenu(row);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void DeviceView::ShowSongs(const SongList &songs) {
  Clear();
  GtkWidget *back = gtk_list_box_row_new();
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(back), gtk_label_new(Translations::CStr("← Devices")));
  g_object_set_data(G_OBJECT(back), "row-kind", const_cast<char *>("back"));
  gtk_list_box_append(GTK_LIST_BOX(list_), back);
  GtkWidget *add_all = gtk_list_box_row_new();
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(add_all), gtk_label_new(Translations::CStr("Add all to playlist")));
  g_object_set_data(G_OBJECT(add_all), "row-kind", const_cast<char *>("add-all"));
  gtk_list_box_append(GTK_LIST_BOX(list_), add_all);
  if (songs.empty()) {
    GtkWidget *empty = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(empty), gtk_label_new(Translations::CStr("No songs found on this device")));
    gtk_list_box_append(GTK_LIST_BOX(list_), empty);
    return;
  }
  for (const Song &song : songs) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(song.PrettyTitleWithArtist().c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    auto *copy = new Song(song);
    g_object_set_data_full(G_OBJECT(row), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    AttachMenu(row);
    SetupRowDrag(row, song);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
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
        if (auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "song"))) {
          static_cast<SongList *>(data)->push_back(*song);
        }
      },
      &songs);
  return songs;
}
