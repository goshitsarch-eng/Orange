#include "widgets/favoritewidget.h"

FavoriteWidget::FavoriteWidget(int tab_index, bool favorite) : tab_index_(tab_index), favorite_(favorite) {
  widget_ = gtk_button_new_from_icon_name("non-starred-symbolic");
  gtk_widget_add_css_class(widget_, "flat");
  g_object_set_data(G_OBJECT(widget_), "tab-part", const_cast<char *>("favorite"));
  Refresh();
  GtkGesture *click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(click));
  g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint n_press, gdouble, gdouble, gpointer data) {
                     if (n_press == 2) {
                       static_cast<FavoriteWidget *>(data)->Toggle();
                     }
                   }),
                   this);
}

void FavoriteWidget::SetFavorite(bool favorite) {
  if (favorite_ == favorite) {
    return;
  }
  favorite_ = favorite;
  Refresh();
  if (changed_) {
    changed_(tab_index_, favorite_);
  }
}

void FavoriteWidget::Toggle() { SetFavorite(!favorite_); }

void FavoriteWidget::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void FavoriteWidget::Refresh() {
  gtk_button_set_icon_name(GTK_BUTTON(widget_), favorite_ ? "starred-symbolic" : "non-starred-symbolic");
  gtk_widget_set_tooltip_text(widget_, TooltipText());
}
