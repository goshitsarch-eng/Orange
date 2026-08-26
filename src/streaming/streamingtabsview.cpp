#include "streaming/streamingtabsview.h"

StreamingTabsView::StreamingTabsView(StreamingService *service) : service_(service) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *stack = gtk_stack_new();
  GtkWidget *switcher = gtk_stack_switcher_new();
  gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
  gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(switcher, 4);
  artists_ = std::make_unique<StreamingCollectionViewContainer>("Artists");
  albums_ = std::make_unique<StreamingCollectionViewContainer>("Albums");
  songs_ = std::make_unique<StreamingCollectionViewContainer>("Songs");
  search_ = std::make_unique<StreamingSearchView>(service);
  gtk_stack_add_titled(GTK_STACK(stack), artists_->widget(), "artists", "Artists");
  gtk_stack_add_titled(GTK_STACK(stack), albums_->widget(), "albums", "Albums");
  gtk_stack_add_titled(GTK_STACK(stack), songs_->widget(), "songs", "Songs");
  gtk_stack_add_titled(GTK_STACK(stack), search_->widget(), "search", "Search");
  gtk_widget_set_vexpand(stack, TRUE);
  gtk_box_append(GTK_BOX(widget_), switcher);
  gtk_box_append(GTK_BOX(widget_), stack);
  artists_->view()->SetRefreshCallback([this]() { GetArtists(); });
  albums_->view()->SetRefreshCallback([this]() { GetAlbums(); });
  songs_->view()->SetRefreshCallback([this]() { GetSongs(); });
}

StreamingTabsView::~StreamingTabsView() = default;

void StreamingTabsView::SetActivateCallback(ActivateCallback callback) {
  activate_ = std::move(callback);
  artists_->view()->SetActivateCallback(activate_);
  albums_->view()->SetActivateCallback(activate_);
  songs_->view()->SetActivateCallback(activate_);
  search_->SetActivateCallback(activate_);
}

void StreamingTabsView::ReloadSettings() {
  if (service_) {
    service_->ReloadSettings();
  }
}

void StreamingTabsView::GetArtists() {
  if (!service_) {
    return;
  }
  artists_->view()->SetStatus("Loading artists…");
  service_->GetArtists([this](const SongList &songs) {
    artists_->view()->SetSongs(songs);
    if (songs.empty()) {
      artists_->view()->SetStatus(service_->logged_in() ? "No artists" : "Sign in in Preferences");
    }
  });
}

void StreamingTabsView::GetAlbums() {
  if (!service_) {
    return;
  }
  albums_->view()->SetStatus("Loading albums…");
  service_->GetAlbums([this](const SongList &songs) {
    albums_->view()->SetSongs(songs);
    if (songs.empty()) {
      albums_->view()->SetStatus(service_->logged_in() ? "No albums" : "Sign in in Preferences");
    }
  });
}

void StreamingTabsView::GetSongs() {
  if (!service_) {
    return;
  }
  songs_->view()->SetStatus("Loading songs…");
  service_->GetSongs([this](const SongList &songs) {
    songs_->view()->SetSongs(songs);
    if (songs.empty()) {
      songs_->view()->SetStatus(service_->logged_in() ? "No songs" : "Sign in in Preferences");
    }
  });
}
