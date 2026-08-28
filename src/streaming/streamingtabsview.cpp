#include "streaming/streamingtabsview.h"

#include "collection/collectiongrouping.h"
#include "core/settings.h"
#include "streaming/streamingbrowse.h"
#include "streaming/streamingfavoriteaction.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchopts.h"
#include "translations/translations.h"

StreamingTabsView::StreamingTabsView(StreamingService *service, Database *database) : service_(service), database_(database) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  stack_ = gtk_stack_new();
  GtkWidget *switcher = gtk_stack_switcher_new();
  gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack_));
  gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(switcher, 4);
  artists_ = std::make_unique<StreamingCollectionViewContainer>("Artists");
  albums_ = std::make_unique<StreamingCollectionViewContainer>("Albums");
  songs_ = std::make_unique<StreamingCollectionViewContainer>("Songs");
  favorites_ = std::make_unique<StreamingCollectionViewContainer>("Favorites");
  search_ = std::make_unique<StreamingSearchView>(service);
  gtk_stack_add_titled(GTK_STACK(stack_), artists_->widget(), "artists", "Artists");
  gtk_stack_add_titled(GTK_STACK(stack_), albums_->widget(), "albums", "Albums");
  gtk_stack_add_titled(GTK_STACK(stack_), songs_->widget(), "songs", "Songs");
  gtk_stack_add_titled(GTK_STACK(stack_), favorites_->widget(), "favorites", "Favorites");
  gtk_stack_add_titled(GTK_STACK(stack_), search_->widget(), "search", "Search");
  gtk_widget_set_vexpand(stack_, TRUE);
  gtk_box_append(GTK_BOX(widget_), switcher);
  gtk_box_append(GTK_BOX(widget_), stack_);
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
    artists_->filter_widget()->SetGrouping(grouping);
    albums_->filter_widget()->SetGrouping(grouping);
    songs_->filter_widget()->SetGrouping(grouping);
    favorites_->filter_widget()->SetGrouping(grouping);
  };
  const std::string configure = StreamingSearchOpts::ConfigureServiceLabel(service_ ? service_->name() : std::string());
  artists_->filter_widget()->SetConfigureLabel(configure);
  albums_->filter_widget()->SetConfigureLabel(configure);
  songs_->filter_widget()->SetConfigureLabel(configure);
  favorites_->filter_widget()->SetConfigureLabel(configure);
  artists_->view()->SetGroupingChangedCallback(sync_grouping);
  albums_->view()->SetGroupingChangedCallback(sync_grouping);
  songs_->view()->SetGroupingChangedCallback(sync_grouping);
  favorites_->view()->SetGroupingChangedCallback(sync_grouping);
  artists_->SetAbortCallback([this]() { AbortGetArtists(); });
  albums_->SetAbortCallback([this]() { AbortGetAlbums(); });
  songs_->SetAbortCallback([this]() { AbortGetSongs(); });
  favorites_->SetAbortCallback([this]() { AbortGetFavorites(); });
  LoadFavoriteType();
  BuildFavoriteTypes();
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
  service_->FavoritesUpdateStatus.Connect([this, alive](const std::string &text) {
    if (alive && *alive) {
      favorites_->SetProgressStatus(text);
      favorites_->ShowProgress();
    }
  });
  service_->FavoritesProgressSetMaximum.Connect([this, alive](int maximum) {
    if (alive && *alive) {
      favorites_->SetProgressMaximum(maximum);
    }
  });
  service_->FavoritesUpdateProgress.Connect([this, alive](int value) {
    if (alive && *alive) {
      favorites_->SetProgress(value);
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

void StreamingTabsView::SetEnqueueCallback(EnqueueCallback callback) {
  artists_->view()->SetEnqueueCallback(callback);
  albums_->view()->SetEnqueueCallback(callback);
  songs_->view()->SetEnqueueCallback(callback);
  favorites_->view()->SetEnqueueCallback(callback);
  search_->SetEnqueueCallback(std::move(callback));
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
  auto collection = [callback](const SongList &songs) { callback(songs, StreamingCollectionActions::MenuContext::Collection); };
  artists_->view()->SetMenuCallback(collection);
  albums_->view()->SetMenuCallback(collection);
  songs_->view()->SetMenuCallback(collection);
  favorites_->view()->SetMenuCallback(collection);
  search_->SetMenuCallback([callback](const SongList &songs) { callback(songs, StreamingCollectionActions::MenuContext::Search); });
}

void StreamingTabsView::SetConfigureCallback(ConfigureCallback callback) { search_->SetConfigureCallback(std::move(callback)); }

void StreamingTabsView::SetFilterMenuCallback(CollectionFilterWidget::MenuActionCallback callback) {
  artists_->SetMenuActionCallback(callback);
  albums_->SetMenuActionCallback(callback);
  songs_->SetMenuActionCallback(callback);
  favorites_->SetMenuActionCallback(std::move(callback));
}

std::string StreamingTabsView::SelectedSearchQuery() const {
  if (!stack_) {
    return {};
  }
  const char *name = gtk_stack_get_visible_child_name(GTK_STACK(stack_));
  if (!name) {
    return {};
  }
  if (std::string(name) == "search") {
    return search_->SelectedSearchQuery();
  }
  if (std::string(name) == "artists") {
    return artists_->view()->SelectedSearchQuery();
  }
  if (std::string(name) == "albums") {
    return albums_->view()->SelectedSearchQuery();
  }
  if (std::string(name) == "songs") {
    return songs_->view()->SelectedSearchQuery();
  }
  if (std::string(name) == "favorites") {
    return favorites_->view()->SelectedSearchQuery();
  }
  return {};
}

void StreamingTabsView::SearchForThis(const std::string &query) {
  const std::string text = query.empty() ? SelectedSearchQuery() : query;
  if (!StreamingSearchOpts::CanSearchForThis(text)) {
    return;
  }
  if (stack_) {
    gtk_stack_set_visible_child_name(GTK_STACK(stack_), "search");
  }
  search_->SearchForThis(text);
}

void StreamingTabsView::ReloadSettings() {
  if (service_) {
    service_->ReloadSettings();
  }
  LoadFavoriteType();
  if (search_ && StreamingSearchOpts::ShouldReloadOnSettingsClose()) {
    search_->ReloadSettings();
  }
  ApplyLook();
}

void StreamingTabsView::ApplyLook() {
  if (artists_) {
    artists_->ApplyLook();
  }
  if (albums_) {
    albums_->ApplyLook();
  }
  if (songs_) {
    songs_->ApplyLook();
  }
  if (favorites_) {
    favorites_->ApplyLook();
  }
  if (search_) {
    search_->ApplyLook();
  }
}

void StreamingTabsView::LoadFavoriteType() {
  if (!service_) {
    favorite_type_ = StreamingService::FavoriteType::Songs;
    return;
  }
  Settings settings;
  settings.BeginGroup(service_->name());
  favorite_type_ = StreamingFavoriteAction::FromInt(
      settings.IntValue(StreamingFavoriteAction::kFavoritesType, StreamingFavoriteAction::ToInt(StreamingService::FavoriteType::Songs)));
}

void StreamingTabsView::PersistFavoriteType() {
  if (!service_) {
    return;
  }
  Settings settings;
  settings.BeginGroup(service_->name());
  settings.SetIntValue(StreamingFavoriteAction::kFavoritesType, StreamingFavoriteAction::ToInt(favorite_type_));
  settings.Sync();
}

void StreamingTabsView::SetFavoriteType(StreamingService::FavoriteType type, bool reload) {
  favorite_type_ = type;
  PersistFavoriteType();
  if (fav_artists_) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fav_artists_), type == StreamingService::FavoriteType::Artists);
  }
  if (fav_albums_) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fav_albums_), type == StreamingService::FavoriteType::Albums);
  }
  if (fav_songs_) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fav_songs_), type == StreamingService::FavoriteType::Songs);
  }
  if (reload) {
    GetFavorites();
  }
}

void StreamingTabsView::BuildFavoriteTypes() {
  GtkWidget *types = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(types, 8);
  gtk_widget_set_margin_end(types, 8);
  gtk_widget_set_margin_top(types, 4);
  gtk_widget_set_margin_bottom(types, 4);
  fav_artists_ = gtk_toggle_button_new_with_label(Translations::CStr(StreamingFavoriteAction::Label(StreamingService::FavoriteType::Artists)));
  fav_albums_ = gtk_toggle_button_new_with_label(Translations::CStr(StreamingFavoriteAction::Label(StreamingService::FavoriteType::Albums)));
  fav_songs_ = gtk_toggle_button_new_with_label(Translations::CStr(StreamingFavoriteAction::Label(StreamingService::FavoriteType::Songs)));
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(fav_albums_), GTK_TOGGLE_BUTTON(fav_artists_));
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(fav_songs_), GTK_TOGGLE_BUTTON(fav_artists_));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fav_artists_), favorite_type_ == StreamingService::FavoriteType::Artists);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fav_albums_), favorite_type_ == StreamingService::FavoriteType::Albums);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fav_songs_), favorite_type_ == StreamingService::FavoriteType::Songs);
  gtk_box_append(GTK_BOX(types), fav_artists_);
  gtk_box_append(GTK_BOX(types), fav_albums_);
  gtk_box_append(GTK_BOX(types), fav_songs_);
  auto on_toggle = +[](GtkToggleButton *button, gpointer data) {
    if (!gtk_toggle_button_get_active(button)) {
      return;
    }
    auto *self = static_cast<StreamingTabsView *>(data);
    if (GTK_WIDGET(button) == self->fav_artists_) {
      self->SetFavoriteType(StreamingService::FavoriteType::Artists, true);
    } else if (GTK_WIDGET(button) == self->fav_albums_) {
      self->SetFavoriteType(StreamingService::FavoriteType::Albums, true);
    } else {
      self->SetFavoriteType(StreamingService::FavoriteType::Songs, true);
    }
  };
  g_signal_connect(fav_artists_, "toggled", G_CALLBACK(on_toggle), this);
  g_signal_connect(fav_albums_, "toggled", G_CALLBACK(on_toggle), this);
  g_signal_connect(fav_songs_, "toggled", G_CALLBACK(on_toggle), this);
  gtk_box_prepend(GTK_BOX(favorites_->widget()), types);
}

void StreamingTabsView::ShowCached(StreamingCollectionView *view, StreamingCollectionStore::List list) {
  if (!view || !service_ || !database_) {
    return;
  }
  const SongList cached =
      service_->WithCoverUrls(StreamingCollectionStore::Load(database_, StreamingCollectionStore::TableName(service_->name(), list)));
  if (!cached.empty()) {
    view->SetSongs(cached);
  }
}

void StreamingTabsView::AddToCollection(StreamingCollectionStore::List list, const SongList &songs) {
  if (!service_ || !database_ || songs.empty() || !StreamingCollectionStore::CanStore(service_->name(), list)) {
    return;
  }
  StreamingCollectionStore::Merge(database_, StreamingCollectionStore::TableName(service_->name(), list), songs);
  StreamingCollectionView *view = songs_->view();
  if (list == StreamingCollectionStore::List::Artists) {
    view = artists_->view();
  } else if (list == StreamingCollectionStore::List::Albums) {
    view = albums_->view();
  }
  ShowCached(view, list);
}

void StreamingTabsView::RemoveFromCollection(StreamingCollectionStore::List list, const SongList &songs) {
  if (!service_ || !database_ || songs.empty() || !StreamingCollectionStore::CanStore(service_->name(), list)) {
    return;
  }
  StreamingCollectionStore::Remove(database_, StreamingCollectionStore::TableName(service_->name(), list), songs);
  StreamingCollectionView *view = songs_->view();
  if (list == StreamingCollectionStore::List::Artists) {
    view = artists_->view();
  } else if (list == StreamingCollectionStore::List::Albums) {
    view = albums_->view();
  }
  if (view) {
    view->SetSongs(
        service_->WithCoverUrls(StreamingCollectionStore::Load(database_, StreamingCollectionStore::TableName(service_->name(), list))));
  }
}

bool StreamingTabsView::CurrentStoreList(StreamingCollectionStore::List *list) const {
  if (!stack_ || !list) {
    return false;
  }
  return StreamingCollectionStore::ListFromTab(gtk_stack_get_visible_child_name(GTK_STACK(stack_)), list);
}

CollectionFilterWidget *StreamingTabsView::CurrentFilterWidget() const {
  if (!stack_) {
    return nullptr;
  }
  const char *name = gtk_stack_get_visible_child_name(GTK_STACK(stack_));
  if (!StreamingCollectionActions::HasDisplayOptionsTab(name)) {
    return nullptr;
  }
  const std::string tab(name);
  if (tab == "artists") {
    return artists_->filter_widget();
  }
  if (tab == "albums") {
    return albums_->filter_widget();
  }
  if (tab == "songs") {
    return songs_->filter_widget();
  }
  if (tab == "favorites") {
    return favorites_->filter_widget();
  }
  return nullptr;
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

void StreamingTabsView::AbortGetFavorites() {
  if (service_) {
    service_->ResetFavoritesRequest();
  }
  favorites_->HideProgress();
}

void StreamingTabsView::GetFavorites() {
  if (!service_) {
    return;
  }
  ShowCached(favorites_->view(), StreamingFavoriteAction::StoreList(favorite_type_));
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    favorites_->ShowProgress(StreamingFavoriteAction::Receiving(favorite_type_));
    service_->StartFavoritesProgress(favorite_type_);
  }
  favorites_->view()->SetStatus("Loading favorites…");
  service_->GetFavorites(favorite_type_, [this](const SongList &songs) {
    favorites_->HideProgressUnlessError();
    if (StreamingCollectionStore::ShouldKeepCache(favorites_->has_error(), songs)) {
      return;
    }
    if (StreamingCollectionStore::ShouldPersist(favorites_->has_error(), service_->logged_in(), songs)) {
      PersistList(StreamingFavoriteAction::StoreList(favorite_type_), songs);
    }
    favorites_->view()->SetSongs(songs);
    if (songs.empty()) {
      favorites_->view()->SetStatus(StreamingFavoriteAction::EmptyStatus(favorite_type_, service_->logged_in()));
    }
  });
}

bool StreamingTabsView::SearchFieldHasFocus() const {
  if (!stack_) {
    return false;
  }
  const char *name = gtk_stack_get_visible_child_name(GTK_STACK(stack_));
  if (!name) {
    return false;
  }
  const std::string tab(name);
  if (tab == "artists") {
    return artists_->view()->SearchFieldHasFocus();
  }
  if (tab == "albums") {
    return albums_->view()->SearchFieldHasFocus();
  }
  if (tab == "songs") {
    return songs_->view()->SearchFieldHasFocus();
  }
  if (tab == "search") {
    return search_->SearchFieldHasFocus();
  }
  return false;
}

void StreamingTabsView::FocusSearchField() {
  if (!stack_) {
    return;
  }
  const char *name = gtk_stack_get_visible_child_name(GTK_STACK(stack_));
  if (!name) {
    return;
  }
  const std::string tab(name);
  if (tab == "artists") {
    artists_->view()->FocusFilter();
  } else if (tab == "albums") {
    albums_->view()->FocusFilter();
  } else if (tab == "songs") {
    songs_->view()->FocusFilter();
  } else if (tab == "search") {
    search_->FocusSearch();
  }
}
