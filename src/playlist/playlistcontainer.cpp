#include "playlist/playlistcontainer.h"

PlaylistContainer::PlaylistContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)),
      tab_bar_(std::make_unique<PlaylistTabBar>()),
      view_(std::make_unique<PlaylistView>()) {
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(toolbar, 8);
  gtk_widget_set_margin_end(toolbar, 8);
  gtk_widget_set_margin_top(toolbar, 8);
  gtk_widget_set_margin_bottom(toolbar, 4);
  auto add_tool = [&](const char *icon, const char *tooltip, const char *name) {
    GtkWidget *button = gtk_button_new_from_icon_name(icon);
    gtk_widget_set_tooltip_text(button, tooltip);
    g_object_set_data_full(G_OBJECT(button), "action", g_strdup(name), g_free);
    gtk_box_append(GTK_BOX(toolbar), button);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistContainer *>(data);
                       const char *action = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "action"));
                       auto *cb = static_cast<ActionCallback *>(g_object_get_data(G_OBJECT(self->widget_), action ? action : ""));
                       if (cb && *cb) {
                         (*cb)();
                       }
                     }),
                     this);
  };
  add_tool("document-new-symbolic", "New playlist", "new");
  add_tool("document-open-symbolic", "Load playlist", "load");
  add_tool("document-save-symbolic", "Save playlist", "save");
  add_tool("edit-clear-all-symbolic", "Clear playlist", "clear");
  add_tool("edit-undo-symbolic", "Undo", "undo");
  add_tool("edit-redo-symbolic", "Redo", "redo");
  repeat_button_ = gtk_button_new_from_icon_name("media-playlist-repeat-symbolic");
  gtk_widget_set_tooltip_text(repeat_button_, "Cycle repeat");
  shuffle_button_ = gtk_button_new_from_icon_name("media-playlist-shuffle-symbolic");
  gtk_widget_set_tooltip_text(shuffle_button_, "Shuffle playlist");
  gtk_box_append(GTK_BOX(toolbar), repeat_button_);
  gtk_box_append(GTK_BOX(toolbar), shuffle_button_);
  GtkWidget *filter = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(filter), "Filter playlist");
  gtk_widget_set_hexpand(filter, TRUE);
  g_signal_connect(filter, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<PlaylistContainer *>(data);
                     self->filter_ = gtk_editable_get_text(GTK_EDITABLE(entry));
                     if (self->filter_changed_) {
                       self->filter_changed_(self->filter_);
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(toolbar), filter);
  summary_ = gtk_label_new("");
  gtk_widget_add_css_class(summary_, "dim-label");
  gtk_box_append(GTK_BOX(toolbar), summary_);
  gtk_widget_set_margin_start(tab_bar_->widget(), 8);
  gtk_widget_set_margin_end(tab_bar_->widget(), 8);
  gtk_box_append(GTK_BOX(widget_), toolbar);
  gtk_box_append(GTK_BOX(widget_), tab_bar_->widget());
  gtk_box_append(GTK_BOX(widget_), view_->widget());
}

void PlaylistContainer::SetFilterChangedCallback(const std::function<void(const std::string &)> &callback) {
  filter_changed_ = callback;
}

void PlaylistContainer::SetActionCallback(const char *name, ActionCallback callback) {
  auto *cb = new ActionCallback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget_), name, cb, [](gpointer p) { delete static_cast<ActionCallback *>(p); });
}

void PlaylistContainer::SetSummary(const std::string &text) { gtk_label_set_text(GTK_LABEL(summary_), text.c_str()); }
