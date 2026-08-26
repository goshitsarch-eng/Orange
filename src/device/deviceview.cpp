#include "device/deviceview.h"

#include "translations/translations.h"

#include <string>

DeviceView::DeviceView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
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
                       if (self->song_cb_) self->song_cb_(*song);
                       return;
                     }
                     const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "device-id"));
                     if (id && self->device_cb_) {
                       self->device_cb_(id);
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
    g_object_set_data_full(G_OBJECT(row), "device-id", g_strdup(device.unique_id.c_str()), g_free);
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
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}
