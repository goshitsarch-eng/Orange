#include "widgets/favoritewidget.h"

FavoriteWidget::FavoriteWidget(int tab_index, bool favorite) : tab_index_(tab_index), favorite_(favorite) {
  widget_ = gtk_toggle_button_new();
  gtk_widget_add_css_class(widget_, "flat");
  gtk_button_set_icon_name(GTK_BUTTON(widget_), "non-starred-symbolic");
  Refresh();
  g_signal_connect(widget_, "toggled", G_CALLBACK(+[](GtkToggleButton *button, gpointer data) {
                     auto *self = static_cast<FavoriteWidget *>(data);
                     self->favorite_ = gtk_toggle_button_get_active(button);
                     self->Refresh();
                     if (self->changed_) {
                       self->changed_(self->tab_index_, self->favorite_);
                     }
                   }),
                   this);
}

void FavoriteWidget::SetFavorite(bool favorite) {
  favorite_ = favorite;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_), favorite);
  Refresh();
}

void FavoriteWidget::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void FavoriteWidget::Refresh() {
  gtk_button_set_icon_name(GTK_BUTTON(widget_), favorite_ ? "starred-symbolic" : "non-starred-symbolic");
  gtk_widget_set_tooltip_text(widget_, favorite_ ? "Favorite playlist" : "Mark playlist as favorite");
}
