#include "playlist/dynamicplaylistcontrols.h"

DynamicPlaylistControls::DynamicPlaylistControls() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(widget_, 8);
  gtk_widget_set_margin_end(widget_, 8);
  GtkWidget *expand = gtk_button_new_with_label("Expand");
  GtkWidget *repopulate = gtk_button_new_with_label("Repopulate");
  GtkWidget *off = gtk_button_new_with_label("Turn off");
  gtk_box_append(GTK_BOX(widget_), gtk_label_new("Dynamic playlist"));
  gtk_box_append(GTK_BOX(widget_), expand);
  gtk_box_append(GTK_BOX(widget_), repopulate);
  gtk_box_append(GTK_BOX(widget_), off);
  g_signal_connect(expand, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<DynamicPlaylistControls *>(data);
                     if (self->expand_) {
                       self->expand_();
                     }
                   }),
                   this);
  g_signal_connect(repopulate, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<DynamicPlaylistControls *>(data);
                     if (self->repopulate_) {
                       self->repopulate_();
                     }
                   }),
                   this);
  g_signal_connect(off, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<DynamicPlaylistControls *>(data);
                     if (self->turn_off_) {
                       self->turn_off_();
                     }
                   }),
                   this);
  gtk_widget_set_visible(widget_, FALSE);
}

void DynamicPlaylistControls::SetSearch(const SmartPlaylistSearch &search) { search_ = search; }

void DynamicPlaylistControls::SetExpandCallback(ExpandCallback callback) { expand_ = std::move(callback); }

void DynamicPlaylistControls::SetRepopulateCallback(RepopulateCallback callback) { repopulate_ = std::move(callback); }

void DynamicPlaylistControls::SetTurnOffCallback(TurnOffCallback callback) { turn_off_ = std::move(callback); }

void DynamicPlaylistControls::SetVisible(bool visible) { gtk_widget_set_visible(widget_, visible); }
