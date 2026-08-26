#include "playlist/playlisttabbar.h"

PlaylistTabBar::PlaylistTabBar() { widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4); }

void PlaylistTabBar::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void PlaylistTabBar::SetFavoriteCallback(FavoriteCallback callback) { favorite_ = std::move(callback); }

void PlaylistTabBar::Refresh(PlaylistManager *manager) {
  favorites_.clear();
  GtkWidget *child = gtk_widget_get_first_child(widget_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
  if (!manager) {
    return;
  }
  int index = 0;
  for (const auto &playlist : manager->playlists()) {
    GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *button = gtk_toggle_button_new_with_label(playlist->name().c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), playlist.get() == manager->current());
    g_object_set_data_full(G_OBJECT(button), "playlist-name", g_strdup(playlist->name().c_str()), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistTabBar *>(data);
                       const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "playlist-name"));
                       if (name && self->changed_) {
                         self->changed_(name);
                       }
                     }),
                     this);
    auto favorite = std::make_unique<FavoriteWidget>(index, playlist->favorite());
    favorite->SetChangedCallback([this, name = playlist->name()](int, bool on) {
      if (favorite_) {
        favorite_(name, on);
      }
    });
    GtkWidget *close = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(close, "flat");
    gtk_widget_add_css_class(close, "circular");
    gtk_widget_set_tooltip_text(close, "Close playlist");
    g_object_set_data(G_OBJECT(close), "playlist-id", GINT_TO_POINTER(playlist->id() + 1));
    g_signal_connect(close, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistTabBar *>(data);
                       const int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "playlist-id")) - 1;
                       if (self->close_) {
                         self->close_(id);
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(tab), button);
    gtk_box_append(GTK_BOX(tab), favorite->widget());
    gtk_box_append(GTK_BOX(tab), close);
    gtk_box_append(GTK_BOX(widget_), tab);
    favorites_.push_back(std::move(favorite));
    ++index;
  }
}
