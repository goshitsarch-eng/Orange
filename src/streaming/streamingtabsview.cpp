#include "streaming/streamingtabsview.h"

#include "collection/collectiongrouping.h"
#include "streaming/streamingbrowse.h"
#include "streaming/streamingprogress.h"

StreamingTabsView::StreamingTabsView(StreamingService *service, Database *database) : service_(service), database_(database) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *stack = gtk_stack_new();
  GtkWidget *switcher = gtk_stack_switcher_new();
  gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
  gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(switcher, 4);
  artists_ = std::make_unique<StreamingCollectionViewContainer>("Artists");
  albums_ = std::make_unique<StreamingCollectionViewContainer>("Albums");
  songs_ = std::make_unique<StreamingCollectionViewContainer>("Songs");
  favorites_ = std::make_unique<StreamingCollectionViewContainer>("Favorites");
  search_ = std::make_unique<StreamingSearchView>(service);
  gtk_stack_add_titled(GTK_STACK(stack), artists_->widget(), "artists", "Artists");
  gtk_stack_add_titled(GTK_STACK(stack), albums_->widget(), "albums", "Albums");
  gtk_stack_add_titled(GTK_STACK(stack), songs_->widget(), "songs", "Songs");
  gtk_stack_add_titled(GTK_STACK(stack), favorites_->widget(), "favorites", "Favorites");
  gtk_stack_add_titled(GTK_STACK(stack), search_->widget(), "search", "Search");
  gtk_widget_set_vexpand(stack, TRUE);
  gtk_box_append(GTK_BOX(widget_), switcher);
  gtk_box_append(GTK_BOX(widget_), stack);
  artists_->view()->SetService(service_);
  albums_->view()->SetService(service_);
  songs_->view()->SetService(service_);
  favorites_->view()->SetService(service_);
  artists_->view()->SetRefreshCallback([this]() { GetArtists(); });
  albums_->view()->SetRefreshCallback([this]() { GetAlbums(); });
  songs_->view()->SetRefreshCallback([this]() { GetSongs(); });
  favorites_->view()->SetRefreshCallback([this]() { GetFavorites(); });
  const auto sync_grouping = [this](const CollectionGrouping::Grouping &grouping) {
    artists_->view()->SetGrouping(grouping);
    albums_->view()->SetGrouping(grouping);
    songs_->view()->SetGrouping(grouping);
    favorites_->view()->SetGrouping(grouping);
  };
  artists_->view()->SetGroupingChangedCallback(sync_grouping);
  albums_->view()->SetGroupingChangedCallback(sync_grouping);
  songs_->view()->SetGroupingChangedCallback(sync_grouping);
  favorites_->view()->SetGroupingChangedCallback(sync_grouping);
  artists_->SetAbortCallback([this]() { AbortGetArtists(); });
  albums_->SetAbortCallback([this]() { AbortGetAlbums(); });
  songs_->SetAbortCallback([this]() { AbortGetSongs(); });
  favorites_->SetAbortCallback([this]() { favorites_->HideProgress(); });
  ConnectBrowseProgress();
}

StreamingTabsView::~StreamingTabsView() {
  if (alive_) {
    *alive_ = false;
  }
}

void StreamingTabsView::ConnectBrowseProgress() {
  if (!service_) {
    return;
  }
  const auto alive = alive_;
  service_->ArtistsUpdateStatus.Connect([this, alive](const std::string &text) {
    if (alive && *alive) {
      artists_->SetProgressStatus(text);
      artists_->ShowProgress();
    }
  });
  service_->ArtistsProgressSetMaximum.Connect([this, alive](int maximum) {
    if (alive && *alive) {
      artists_->SetProgressMaximum(maximum);
    }
  });
  service_->ArtistsUpdateProgress.Connect([this, alive](int value) {
    if (alive && *alive) {
      artists_->SetProgress(value);
    }
  });
  service_->AlbumsUpdateStatus.Connect([this, alive](const std::string &text) {
    if (alive && *alive) {
      albums_->SetProgressStatus(text);
      albums_->ShowProgress();
    }
  });
  service_->AlbumsProgressSetMaximum.Connect([this, alive](int maximum) {
    if (alive && *alive) {
      albums_->SetProgressMaximum(maximum);
    }
  });
  service_->AlbumsUpdateProgress.Connect([this, alive](int value) {
    if (alive && *alive) {
      albums_->SetProgress(value);
    }
  });
  service_->SongsUpdateStatus.Connect([this, alive](const std::string &text) {
    if (alive && *alive) {
      songs_->SetProgressStatus(text);
      songs_->ShowProgress();
    }
  });
  service_->SongsProgressSetMaximum.Connect([this, alive](int maximum) {
    if (alive && *alive) {
      songs_->SetProgressMaximum(maximum);
    }
  });
  service_->SongsUpdateProgress.Connect([this, alive](int value) {
    if (alive && *alive) {
      songs_->SetProgress(value);
    }
  });
  service_->ArtistsFailed.Connect([this, alive](const std::string &error) {
    if (alive && *alive) {
      artists_->ShowError(error);
    }
  });
  service_->AlbumsFailed.Connect([this, alive](const std::string &error) {
    if (alive && *alive) {
      albums_->ShowError(error);
    }
  });
  service_->SongsFailed.Connect([this, alive](const std::string &error) {
    if (alive && *alive) {
      songs_->ShowError(error);
    }
  });
  service_->FavoritesFailed.Connect([this, alive](const std::string &error) {
    if (alive && *alive) {
      favorites_->ShowError(error);
    }
  });
}

void StreamingTabsView::SetActivateCallback(ActivateCallback callback) {
  activate_ = std::move(callback);
  artists_->view()->SetActivateCallback([this](const Song &song) { HandleActivate(artists_->view(), song); });
  albums_->view()->SetActivateCallback([this](const Song &song) { HandleActivate(albums_->view(), song); });
  songs_->view()->SetActivateCallback([this](const Song &song) { HandleActivate(songs_->view(), song); });
  favorites_->view()->SetActivateCallback([this](const Song &song) { HandleActivate(favorites_->view(), song); });
  search_->SetActivateCallback(activate_);
}

void StreamingTabsView::HandleActivate(StreamingCollectionView *view, const Song &song) {
  if (!view) {
    return;
  }
  const StreamingBrowse::Kind kind = StreamingBrowse::KindOf(song);
  if (kind == StreamingBrowse::Kind::Artist) {
    BrowseArtist(view, song);
    return;
  }
  if (kind == StreamingBrowse::Kind::Album) {
    BrowseAlbum(view, song);
    return;
  }
  if (activate_) {
    activate_(song);
  }
}

void StreamingTabsView::BrowseArtist(StreamingCollectionView *view, const Song &artist) {
  if (!service_) {
    return;
  }
  StreamingCollectionViewContainer *container = artists_.get();
  if (view == albums_->view()) {
    container = albums_.get();
  } else if (view == songs_->view()) {
    container = songs_.get();
  } else if (view == favorites_->view()) {
    container = favorites_.get();
  }
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    container->ShowProgress(StreamingProgress::ReceivingAlbums());
  }
  service_->GetArtistAlbums(artist, [this, view, container](const SongList &albums) {
    container->HideProgressUnlessError();
    view->PushSongs(albums);
    if (albums.empty()) {
      view->SetStatus(service_->logged_in() ? "No albums" : "Sign in in Preferences");
    }
  });
}

void StreamingTabsView::BrowseAlbum(StreamingCollectionView *view, const Song &album) {
  if (!service_) {
    return;
  }
  StreamingCollectionViewContainer *container = songs_.get();
  if (view == artists_->view()) {
    container = artists_.get();
  } else if (view == albums_->view()) {
    container = albums_.get();
  } else if (view == favorites_->view()) {
    container = favorites_.get();
  }
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    container->ShowProgress(StreamingProgress::ReceivingSongs());
  }
  service_->GetAlbumSongs(album, [this, view, container](const SongList &songs) {
    container->HideProgressUnlessError();
    view->PushSongs(songs);
    if (songs.empty()) {
      view->SetStatus(service_->logged_in() ? "No songs" : "Sign in in Preferences");
    }
  });
}

void StreamingTabsView::SetMenuCallback(MenuCallback callback) {
  artists_->view()->SetMenuCallback(callback);
  albums_->view()->SetMenuCallback(callback);
  songs_->view()->SetMenuCallback(callback);
  favorites_->view()->SetMenuCallback(callback);
  search_->SetMenuCallback(std::move(callback));
}

void StreamingTabsView::SetConfigureCallback(ConfigureCallback callback) { search_->SetConfigureCallback(std::move(callback)); }

void StreamingTabsView::ReloadSettings() {
  if (service_) {
    service_->ReloadSettings();
  }
}

void StreamingTabsView::ShowCached(StreamingCollectionView *view, StreamingCollectionStore::List list) {
  if (!view || !service_ || !database_) {
    return;
  }
  const SongList cached = StreamingCollectionStore::Load(database_, StreamingCollectionStore::TableName(service_->name(), list));
  if (!cached.empty()) {
    view->SetSongs(cached);
  }
}

void StreamingTabsView::PersistList(StreamingCollectionStore::List list, const SongList &songs) {
  if (!service_ || !database_) {
    return;
  }
  StreamingCollectionStore::Replace(database_, StreamingCollectionStore::TableName(service_->name(), list), songs);
}

void StreamingTabsView::GetArtists() {
  if (!service_) {
    return;
  }
  ShowCached(artists_->view(), StreamingCollectionStore::List::Artists);
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    artists_->ShowProgress(StreamingProgress::ReceivingArtists());
    service_->StartArtistsProgress();
  }
  artists_->view()->SetStatus("Loading artists…");
  service_->GetArtists([this](const SongList &songs) {
    artists_->HideProgressUnlessError();
    if (StreamingCollectionStore::ShouldKeepCache(artists_->has_error(), songs)) {
      return;
    }
    if (StreamingCollectionStore::ShouldPersist(artists_->has_error(), service_->logged_in(), songs)) {
      PersistList(StreamingCollectionStore::List::Artists, songs);
    }
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
  ShowCached(albums_->view(), StreamingCollectionStore::List::Albums);
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    albums_->ShowProgress(StreamingProgress::ReceivingAlbums());
    service_->StartAlbumsProgress();
  }
  albums_->view()->SetStatus("Loading albums…");
  service_->GetAlbums([this](const SongList &songs) {
    albums_->HideProgressUnlessError();
    if (StreamingCollectionStore::ShouldKeepCache(albums_->has_error(), songs)) {
      return;
    }
    if (StreamingCollectionStore::ShouldPersist(albums_->has_error(), service_->logged_in(), songs)) {
      PersistList(StreamingCollectionStore::List::Albums, songs);
    }
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
  ShowCached(songs_->view(), StreamingCollectionStore::List::Songs);
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    songs_->ShowProgress(StreamingProgress::ReceivingSongs());
    service_->StartSongsProgress();
  }
  songs_->view()->SetStatus("Loading songs…");
  service_->GetSongs([this](const SongList &songs) {
    songs_->HideProgressUnlessError();
    if (StreamingCollectionStore::ShouldKeepCache(songs_->has_error(), songs)) {
      return;
    }
    if (StreamingCollectionStore::ShouldPersist(songs_->has_error(), service_->logged_in(), songs)) {
      PersistList(StreamingCollectionStore::List::Songs, songs);
    }
    songs_->view()->SetSongs(songs);
    if (songs.empty()) {
      songs_->view()->SetStatus(service_->logged_in() ? "No songs" : "Sign in in Preferences");
    }
  });
}

void StreamingTabsView::AbortGetArtists() {
  if (service_) {
    service_->ResetArtistsRequest();
  }
  artists_->HideProgress();
}

void StreamingTabsView::AbortGetAlbums() {
  if (service_) {
    service_->ResetAlbumsRequest();
  }
  albums_->HideProgress();
}

void StreamingTabsView::AbortGetSongs() {
  if (service_) {
    service_->ResetSongsRequest();
  }
  songs_->HideProgress();
}

void StreamingTabsView::GetFavorites() {
  if (!service_) {
    return;
  }
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    favorites_->ShowProgress(StreamingProgress::ReceivingSongs());
  }
  favorites_->view()->SetStatus("Loading favorites…");
  service_->GetFavorites(StreamingService::FavoriteType::Songs, [this](const SongList &songs) {
    favorites_->HideProgressUnlessError();
    favorites_->view()->SetSongs(songs);
    if (songs.empty()) {
      favorites_->view()->SetStatus(service_->logged_in() ? "No favorites" : "Sign in in Preferences");
    }
  });
}
