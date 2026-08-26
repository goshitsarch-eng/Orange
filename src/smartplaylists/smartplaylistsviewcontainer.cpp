#include "smartplaylists/smartplaylistsviewcontainer.h"

#include "translations/translations.h"

SmartPlaylistsViewContainer::SmartPlaylistsViewContainer() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(bar, 8);
  gtk_widget_set_margin_end(bar, 8);
  gtk_widget_set_margin_top(bar, 6);
  gtk_widget_set_margin_bottom(bar, 4);
  GtkWidget *add = gtk_button_new_from_icon_name("document-new-symbolic");
  gtk_widget_add_css_class(add, "flat");
  gtk_widget_set_tooltip_text(add, Translations::CStr("New smart playlist…"));
  edit_button_ = gtk_button_new_from_icon_name("document-edit-symbolic");
  gtk_widget_add_css_class(edit_button_, "flat");
  gtk_widget_set_tooltip_text(edit_button_, Translations::CStr("Edit smart playlist…"));
  delete_button_ = gtk_button_new_from_icon_name("edit-delete-symbolic");
  gtk_widget_add_css_class(delete_button_, "flat");
  gtk_widget_set_tooltip_text(delete_button_, Translations::CStr("Delete smart playlist"));
  GtkWidget *restore = gtk_button_new_from_icon_name("view-refresh-symbolic");
  gtk_widget_add_css_class(restore, "flat");
  gtk_widget_set_tooltip_text(restore, Translations::CStr("Restore defaults"));
  g_signal_connect(add, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<SmartPlaylistsViewContainer *>(data)->EmitSelected(SmartPlaylistsAction::New);
                   }),
                   this);
  g_signal_connect(edit_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<SmartPlaylistsViewContainer *>(data)->EmitSelected(SmartPlaylistsAction::Edit);
                   }),
                   this);
  g_signal_connect(delete_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<SmartPlaylistsViewContainer *>(data)->EmitSelected(SmartPlaylistsAction::Delete);
                   }),
                   this);
  g_signal_connect(restore, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<SmartPlaylistsViewContainer *>(data)->EmitSelected(SmartPlaylistsAction::RestoreDefaults);
                   }),
                   this);
  gtk_box_append(GTK_BOX(bar), add);
  gtk_box_append(GTK_BOX(bar), edit_button_);
  gtk_box_append(GTK_BOX(bar), delete_button_);
  gtk_box_append(GTK_BOX(bar), restore);
  view_ = std::make_unique<SmartPlaylistsView>();
  gtk_widget_set_vexpand(view_->widget(), TRUE);
  g_signal_connect(view_->list(), "selected-rows-changed", G_CALLBACK(+[](GtkListBox *, gpointer data) {
                     static_cast<SmartPlaylistsViewContainer *>(data)->UpdateButtons();
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), bar);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  Reload();
}

void SmartPlaylistsViewContainer::Reload() {
  model_.Reload();
  view_->Reload(&model_);
  UpdateButtons();
}

void SmartPlaylistsViewContainer::SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback) {
  view_->SetActivateCallback(std::move(callback));
}

void SmartPlaylistsViewContainer::SetDeleteCallback(std::function<void(const SmartPlaylistsItem &)> callback) {
  view_->SetDeleteCallback(std::move(callback));
}

void SmartPlaylistsViewContainer::SetActionCallback(std::function<void(const SmartPlaylistsItem &, SmartPlaylistsAction)> callback) {
  view_->SetActionCallback(std::move(callback));
}

void SmartPlaylistsViewContainer::SetSongsCallback(SmartPlaylistsView::SongsCallback callback) {
  view_->SetSongsCallback(std::move(callback));
}

void SmartPlaylistsViewContainer::EmitSelected(SmartPlaylistsAction action) { view_->Trigger(action); }

void SmartPlaylistsViewContainer::UpdateButtons() {
  const SmartPlaylistsItem *item = view_->SelectedItem();
  const bool saved = item && item->kind == SmartPlaylistsItem::Kind::Saved;
  gtk_widget_set_sensitive(edit_button_, saved ? TRUE : FALSE);
  gtk_widget_set_sensitive(delete_button_, saved ? TRUE : FALSE);
}
