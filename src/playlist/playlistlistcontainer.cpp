#include "playlist/playlistlistcontainer.h"

#include "translations/translations.h"

PlaylistListContainer::PlaylistListContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)), filter_(&model_), view_(std::make_unique<PlaylistListView>()) {
  search_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_), Translations::CStr("Filter playlists"));
  gtk_widget_set_margin_start(search_, 8);
  gtk_widget_set_margin_end(search_, 8);
  gtk_widget_set_margin_top(search_, 6);
  g_signal_connect(search_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->filter_.SetFilter(text ? text : "");
                     self->Rebuild();
                   }),
                   this);
  GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(bar, 8);
  gtk_widget_set_margin_end(bar, 8);
  gtk_widget_set_margin_bottom(bar, 4);
  GtkWidget *add = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_add_css_class(add, "flat");
  gtk_widget_set_tooltip_text(add, Translations::CStr("New playlist"));
  GtkWidget *remove = gtk_button_new_from_icon_name("list-remove-symbolic");
  gtk_widget_add_css_class(remove, "flat");
  gtk_widget_set_tooltip_text(remove, Translations::CStr("Delete playlist"));
  GtkWidget *save = gtk_button_new_from_icon_name("document-save-symbolic");
  gtk_widget_add_css_class(save, "flat");
  gtk_widget_set_tooltip_text(save, Translations::CStr("Save playlist"));
  GtkWidget *copy = gtk_button_new_from_icon_name("drive-harddisk-usb-symbolic");
  gtk_widget_add_css_class(copy, "flat");
  gtk_widget_set_tooltip_text(copy, Translations::CStr("Copy to device…"));
  favorites_toggle_ = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(favorites_toggle_), "starred-symbolic");
  gtk_widget_add_css_class(favorites_toggle_, "flat");
  gtk_widget_set_tooltip_text(favorites_toggle_, Translations::CStr("Show favorite playlists only"));
  g_signal_connect(add, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->new_) {
                       self->new_();
                     }
                   }),
                   this);
  g_signal_connect(remove, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->delete_) {
                       self->delete_(self->SelectedName());
                     }
                   }),
                   this);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->save_) {
                       self->save_(self->SelectedName());
                     }
                   }),
                   this);
  g_signal_connect(copy, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     if (self->copy_) {
                       self->copy_(self->SelectedName());
                     }
                   }),
                   this);
  g_signal_connect(favorites_toggle_, "toggled", G_CALLBACK(+[](GtkToggleButton *button, gpointer data) {
                     auto *self = static_cast<PlaylistListContainer *>(data);
                     self->filter_.SetFavoritesOnly(gtk_toggle_button_get_active(button) == TRUE);
                     self->Rebuild();
                   }),
                   this);
  gtk_box_append(GTK_BOX(bar), add);
  gtk_box_append(GTK_BOX(bar), remove);
  gtk_box_append(GTK_BOX(bar), save);
  gtk_box_append(GTK_BOX(bar), copy);
  gtk_box_append(GTK_BOX(bar), favorites_toggle_);
  gtk_widget_set_vexpand(view_->widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), search_);
  gtk_box_append(GTK_BOX(widget_), bar);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  view_->SetMenuCallback([this](const std::string &name) {
    if (menu_) {
      menu_(name);
    }
  });
}

void PlaylistListContainer::Reload(PlaylistManager *manager) {
  manager_ = manager;
  model_.Reload(manager);
  Rebuild();
}

void PlaylistListContainer::Rebuild() {
  std::string current;
  if (manager_ && manager_->current()) {
    current = manager_->current()->name();
  }
  view_->Refresh(filter_.VisibleRows(), current);
}

void PlaylistListContainer::SetActivateCallback(const std::function<void(const std::string &)> &callback) {
  view_->SetActivateCallback(callback);
}

void PlaylistListContainer::SetDropCallback(DropCallback callback) { view_->SetDropCallback(std::move(callback)); }

std::string PlaylistListContainer::SelectedName() const { return view_->SelectedName(); }

void PlaylistListContainer::ApplyFilter() { Rebuild(); }
