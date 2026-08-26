#include "streaming/streamingsearchview.h"

#include "collection/collectionfiltermenu.h"
#include "collection/groupbydialog.h"
#include "core/settings.h"
#include "dialogs/dialoghelpers.h"
#include "streaming/streamingabort.h"
#include "streaming/streamingcover.h"
#include "streaming/streamingdrag.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchopts.h"
#include "streaming/streamingsearchgroup.h"
#include "utilities/fileutils.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "translations/translations.h"
#include "utilities/jsonutils.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"

StreamingSearchView::StreamingSearchView(StreamingService *service) : service_(service) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  search_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_entry_), Translations::CStr("Search"));
  gtk_widget_set_margin_start(search_entry_, 8);
  gtk_widget_set_margin_end(search_entry_, 8);
  gtk_widget_set_margin_top(search_entry_, 6);
  gtk_widget_set_margin_bottom(search_entry_, 4);
  GtkWidget *types = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(types, 8);
  gtk_widget_set_margin_end(types, 8);
  gtk_widget_set_margin_bottom(types, 4);
  type_artists_ = gtk_toggle_button_new_with_label("Artists");
  type_albums_ = gtk_toggle_button_new_with_label("Albums");
  type_songs_ = gtk_toggle_button_new_with_label("Songs");
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(type_albums_), GTK_TOGGLE_BUTTON(type_artists_));
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(type_songs_), GTK_TOGGLE_BUTTON(type_artists_));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(type_songs_), TRUE);
  gtk_box_append(GTK_BOX(types), type_artists_);
  gtk_box_append(GTK_BOX(types), type_albums_);
  gtk_box_append(GTK_BOX(types), type_songs_);
  pretty_covers_btn_ = gtk_check_button_new_with_label(Translations::CStr("Pretty covers"));
  if (service_) {
    Settings settings;
    settings.BeginGroup(service_->name());
    pretty_covers_ = settings.BoolValue(StreamingCover::kPrettyCovers, StreamingCover::kDefaultPrettyCovers);
    grouping_ = StreamingSearchGroup::FromSaved(settings.IntValue(StreamingSearchGroup::kSearchGroupBy1, 0),
                                                settings.IntValue(StreamingSearchGroup::kSearchGroupBy2, 0),
                                                settings.IntValue(StreamingSearchGroup::kSearchGroupBy3, 0),
                                                settings.Contains(StreamingSearchGroup::kSearchGroupBy1));
  }
  gtk_check_button_set_active(GTK_CHECK_BUTTON(pretty_covers_btn_), pretty_covers_);
  gtk_widget_set_hexpand(pretty_covers_btn_, TRUE);
  gtk_widget_set_halign(pretty_covers_btn_, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(types), pretty_covers_btn_);
  g_signal_connect(pretty_covers_btn_, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     self->pretty_covers_ = gtk_check_button_get_active(button);
                     self->PersistPrettyCovers();
                     self->Rebuild();
                   }),
                   this);
  group_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(group_button_), "view-list-symbolic");
  gtk_widget_set_tooltip_text(group_button_, Translations::CStr("Group by"));
  gtk_box_append(GTK_BOX(types), group_button_);
  BuildGroupMenu();
  configure_button_ = gtk_button_new_from_icon_name("emblem-system-symbolic");
  gtk_widget_set_tooltip_text(configure_button_, Translations::CStr(StreamingSearchOpts::ConfigureLabel()));
  gtk_box_append(GTK_BOX(types), configure_button_);
  g_signal_connect(configure_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     if (self->configure_) {
                       self->configure_();
                     }
                   }),
                   this);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  auto search_now = +[](GtkWidget *, gpointer data) {
    auto *self = static_cast<StreamingSearchView *>(data);
    const char *text = gtk_editable_get_text(GTK_EDITABLE(self->search_entry_));
    self->ScheduleSearch(text ? text : "", true);
  };
  g_signal_connect(search_entry_, "activate", G_CALLBACK(search_now), this);
  g_signal_connect(search_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     const std::string query = text ? text : "";
                     if (!StreamingProgress::HasQuery(query)) {
                       self->CancelPendingSearch();
                       self->Search({});
                       return;
                     }
                     if (!StreamingSearchOpts::ShouldSearchOnChange(query)) {
                       self->CancelPendingSearch();
                       return;
                     }
                     self->ScheduleSearch(query, false);
                   }),
                   this);
  g_signal_connect(type_artists_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(type_albums_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(type_songs_, "toggled", G_CALLBACK(search_now), this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song && self->activate_) {
                       self->activate_(*song);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     if (self->menu_) {
                       self->menu_(self->SelectedSongs());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  progress_ = gtk_progress_bar_new();
  gtk_widget_set_margin_start(progress_, 8);
  gtk_widget_set_margin_end(progress_, 8);
  gtk_widget_set_visible(progress_, FALSE);
  status_ = gtk_label_new("");
  gtk_widget_set_halign(status_, GTK_ALIGN_START);
  gtk_widget_set_margin_start(status_, 8);
  gtk_widget_set_margin_end(status_, 8);
  gtk_widget_set_visible(status_, FALSE);
  close_ = gtk_button_new_with_label(Translations::CStr(StreamingAbort::CloseLabel()));
  gtk_widget_set_halign(close_, GTK_ALIGN_END);
  gtk_widget_set_margin_start(close_, 8);
  gtk_widget_set_margin_end(close_, 8);
  gtk_widget_set_margin_bottom(close_, 8);
  gtk_widget_set_visible(close_, FALSE);
  g_signal_connect(close_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<StreamingSearchView *>(data)->HideProgress();
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), search_entry_);
  gtk_box_append(GTK_BOX(widget_), types);
  gtk_box_append(GTK_BOX(widget_), progress_);
  gtk_box_append(GTK_BOX(widget_), status_);
  gtk_box_append(GTK_BOX(widget_), close_);
  gtk_box_append(GTK_BOX(widget_), scroll);
  if (service_) {
    const auto alive = alive_;
    service_->SearchUpdateStatus.Connect([this, alive](int id, const std::string &text) {
      if (!alive || !*alive || id != last_search_id_) {
        return;
      }
      ApplyStatus(text);
    });
    service_->SearchProgressSetMaximum.Connect([this, alive](int id, int maximum) {
      if (!alive || !*alive || id != last_search_id_ || maximum <= 0) {
        return;
      }
      progress_max_ = maximum;
    });
    service_->SearchUpdateProgress.Connect([this, alive](int id, int value) {
      if (!alive || !*alive || id != last_search_id_) {
        return;
      }
      ApplyProgress(value, progress_max_);
    });
    service_->SearchFailed.Connect([this, alive](int id, const std::string &error) {
      if (!alive || !*alive || id != last_search_id_) {
        return;
      }
      ShowError(error);
    });
  }
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<StreamingSearchView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

StreamingSearchView::~StreamingSearchView() {
  if (alive_) {
    *alive_ = false;
  }
  CancelPendingSearch();
  ResetTypeAhead();
}

void StreamingSearchView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void StreamingSearchView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void StreamingSearchView::SetConfigureCallback(ConfigureCallback callback) { configure_ = std::move(callback); }

void StreamingSearchView::CancelPendingSearch() {
  ++search_timer_gen_;
  if (search_timer_ != 0) {
    g_source_remove(search_timer_);
    search_timer_ = 0;
  }
}

void StreamingSearchView::ScheduleSearch(const std::string &query, bool immediate) {
  CancelPendingSearch();
  pending_query_ = query;
  const int delay = service_ ? StreamingSearchOpts::DelayMs(service_->name()) : 0;
  if (!StreamingSearchOpts::ShouldDelay(delay, immediate)) {
    Search(query);
    return;
  }
  struct Job {
    StreamingSearchView *view = nullptr;
    int generation = 0;
  };
  auto *job = new Job{this, search_timer_gen_};
  search_timer_ = g_timeout_add_full(
      G_PRIORITY_DEFAULT, static_cast<guint>(delay),
      +[](gpointer data) -> gboolean {
        auto *job = static_cast<Job *>(data);
        if (job->view && job->generation == job->view->search_timer_gen_) {
          job->view->search_timer_ = 0;
          job->view->Search(job->view->pending_query_);
        }
        return G_SOURCE_REMOVE;
      },
      job, +[](gpointer data) { delete static_cast<Job *>(data); });
}

void StreamingSearchView::HideProgress() {
  has_error_ = false;
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    gtk_widget_set_visible(progress_, FALSE);
  }
  if (status_) {
    gtk_label_set_text(GTK_LABEL(status_), "");
    gtk_widget_set_visible(status_, FALSE);
  }
  if (close_) {
    gtk_widget_set_visible(close_, FALSE);
  }
}

void StreamingSearchView::HideProgressUnlessError() {
  if (!has_error_) {
    HideProgress();
  }
}

void StreamingSearchView::ShowError(const std::string &status) {
  has_error_ = true;
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    gtk_widget_set_visible(progress_, FALSE);
  }
  ApplyStatus(status);
  if (progress_) {
    gtk_widget_set_visible(progress_, FALSE);
  }
  if (close_) {
    gtk_widget_set_visible(close_, StreamingAbort::ShouldShowClose(false, has_error_) ? TRUE : FALSE);
  }
}

void StreamingSearchView::ApplyStatus(const std::string &text) {
  if (status_) {
    gtk_label_set_text(GTK_LABEL(status_), text.c_str());
    gtk_widget_set_visible(status_, TRUE);
  }
  if (progress_) {
    gtk_widget_set_visible(progress_, TRUE);
  }
}

void StreamingSearchView::ApplyProgress(int value, int maximum) {
  if (has_error_) {
    return;
  }
  if (maximum > 0) {
    progress_max_ = maximum;
  }
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), StreamingProgress::Fraction(value, progress_max_));
    gtk_widget_set_visible(progress_, TRUE);
  }
}

void StreamingSearchView::Search(const std::string &query) {
  if (!service_ || !StreamingProgress::HasQuery(query)) {
    last_search_id_ = -1;
    if (service_) {
      service_->CancelSearch();
    }
    HideProgress();
    return;
  }
  StreamingService::SearchType type = StreamingService::SearchType::Songs;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_artists_))) {
    type = StreamingService::SearchType::Artists;
  } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_albums_))) {
    type = StreamingService::SearchType::Albums;
  }
  model_.SetSearchType(type);
  ++cover_gen_;
  has_error_ = false;
  if (close_) {
    gtk_widget_set_visible(close_, FALSE);
  }
  last_search_id_ = service_->last_search_id() + 1;
  service_->StartSearchProgress();
  const int gen = cover_gen_;
  const auto alive = alive_;
  service_->Search(query, type, [this, alive, gen](const SongList &songs) {
    if (!alive || !*alive || gen != cover_gen_) {
      return;
    }
    HideProgressUnlessError();
    model_.SetSongs(songs);
    Rebuild();
  });
}

void StreamingSearchView::Rebuild() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList visible = sort_model_.Visible();
  if (visible.empty()) {
    gtk_list_box_append(GTK_LIST_BOX(list_), gtk_label_new(Translations::CStr("No results")));
    return;
  }
  ++cover_gen_;
  const std::vector<StreamingSearchGroup::Row> rows =
      StreamingSearchGroup::RowsFor(visible, grouping_, CollectionGrouping::SeparateAlbumsByGrouping());
  for (const StreamingSearchGroup::Row &entry : rows) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(row_box, StreamingSearchGroup::IndentPixels(entry.indent));
    gtk_widget_set_margin_end(row_box, 8);
    gtk_widget_set_margin_top(row_box, 4);
    gtk_widget_set_margin_bottom(row_box, 4);
    if (entry.header) {
      GtkWidget *label = gtk_label_new(entry.label.c_str());
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_add_css_class(label, "heading");
      gtk_box_append(GTK_BOX(row_box), label);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
      gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
      gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);
      gtk_list_box_append(GTK_LIST_BOX(list_), row);
      continue;
    }
    if (StreamingCover::ShouldShowThumb(pretty_covers_)) {
      GtkWidget *image = gtk_image_new_from_icon_name(StreamingCover::kPlaceholderIcon);
      gtk_image_set_pixel_size(GTK_IMAGE(image), StreamingCover::kArtHeight);
      gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
      gtk_box_append(GTK_BOX(row_box), image);
      LoadCover(image, entry.song);
    }
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(box, TRUE);
    GtkWidget *primary = gtk_label_new(StreamingSearchItemDelegate::PrimaryText(entry.song).c_str());
    gtk_widget_set_halign(primary, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), primary);
    const std::string secondary = StreamingSearchItemDelegate::SecondaryText(entry.song);
    if (!secondary.empty()) {
      GtkWidget *sub = gtk_label_new(secondary.c_str());
      gtk_widget_add_css_class(sub, "dim-label");
      gtk_widget_set_halign(sub, GTK_ALIGN_START);
      gtk_box_append(GTK_BOX(box), sub);
    }
    gtk_box_append(GTK_BOX(row_box), box);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    g_object_set_data_full(G_OBJECT(row), "row-data", new Song(entry.song), [](gpointer p) { delete static_cast<Song *>(p); });
    SetupRowDrag(row, entry.song);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void StreamingSearchView::PersistPrettyCovers() {
  if (!service_) {
    return;
  }
  Settings settings;
  settings.BeginGroup(service_->name());
  settings.SetBoolValue(StreamingCover::kPrettyCovers, pretty_covers_);
  settings.Sync();
}

void StreamingSearchView::PersistGrouping() {
  if (!service_) {
    return;
  }
  Settings settings;
  settings.BeginGroup(service_->name());
  settings.SetIntValue(StreamingSearchGroup::kSearchGroupBy1, static_cast<int>(grouping_.first));
  settings.SetIntValue(StreamingSearchGroup::kSearchGroupBy2, static_cast<int>(grouping_.second));
  settings.SetIntValue(StreamingSearchGroup::kSearchGroupBy3, static_cast<int>(grouping_.third));
  settings.Sync();
}

void StreamingSearchView::ApplyGrouping(const CollectionGrouping::Grouping &grouping) {
  grouping_ = grouping;
  PersistGrouping();
  Rebuild();
}

void StreamingSearchView::BuildGroupMenu() {
  if (!group_button_) {
    return;
  }
  GMenu *menu = g_menu_new();
  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  for (size_t i = 0; i < presets.size(); ++i) {
    if (presets[i].advanced) {
      continue;
    }
    char action[64];
    g_snprintf(action, sizeof(action), "streamsearch.preset(%d)", static_cast<int>(i));
    g_menu_append(menu, Translations::CStr(presets[i].label), action);
  }
  g_menu_append(menu, Translations::CStr("Advanced grouping…"), "streamsearch.advanced");
  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *preset = g_simple_action_new("preset", G_VARIANT_TYPE_INT32);
  g_signal_connect(preset, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     const std::vector<CollectionFilterMenu::Preset> items = CollectionFilterMenu::BuiltinPresets();
                     const int index = g_variant_get_int32(param);
                     if (index < 0 || static_cast<size_t>(index) >= items.size() || items[static_cast<size_t>(index)].advanced) {
                       return;
                     }
                     self->ApplyGrouping(items[static_cast<size_t>(index)].grouping);
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(preset));
  GSimpleAction *advanced = g_simple_action_new("advanced", nullptr);
  g_signal_connect(advanced, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     GtkRoot *root = gtk_widget_get_root(self->widget_);
                     GtkWindow *parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
                     GroupByDialog::Show(parent, self->grouping_, [self](const CollectionGrouping::Grouping &grouping) {
                       self->ApplyGrouping(grouping);
                     });
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(advanced));
  gtk_widget_insert_action_group(group_button_, "streamsearch", G_ACTION_GROUP(group));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(group_button_), G_MENU_MODEL(menu));
  g_object_unref(group);
  g_object_unref(menu);
}

void StreamingSearchView::LoadCover(GtkWidget *image, const Song &song) {
  if (!image) {
    return;
  }
  const std::string key = StreamingCover::CacheKey(song);
  const auto cached = cover_cache_.find(key);
  if (cached != cover_cache_.end()) {
    DialogHelpers::SetImageFromBytes(image, std::vector<unsigned char>(cached->second.begin(), cached->second.end()),
                                     StreamingCover::kArtHeight);
    return;
  }
  const std::string url = StreamingCover::CoverUrl(song);
  if (!StreamingCover::CanLoad(url)) {
    return;
  }
  if (StreamingCover::IsLocalUrl(url)) {
    const std::string body = FileUtils::ReadFile(FileUtils::PathFromUri(url));
    if (body.empty() || !JsonUtils::LooksLikeImage(body)) {
      return;
    }
    cover_cache_[key] = body;
    DialogHelpers::SetImageFromBytes(image, std::vector<unsigned char>(body.begin(), body.end()), StreamingCover::kArtHeight);
    return;
  }
  if (!service_ || !service_->network()) {
    return;
  }
  const int gen = cover_gen_;
  const auto alive = alive_;
  service_->network()->Get(url, [this, alive, gen, image, key](const NetworkAccessManager::Response &response) {
    if (!alive || !*alive || gen != cover_gen_ || !image) {
      return;
    }
    if (!response.ok() || !JsonUtils::LooksLikeImage(response.body)) {
      return;
    }
    cover_cache_[key] = response.body;
    DialogHelpers::SetImageFromBytes(image, std::vector<unsigned char>(response.body.begin(), response.body.end()),
                                     StreamingCover::kArtHeight);
  });
}

void StreamingSearchView::SetupRowDrag(GtkWidget *row, const Song &song) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  g_object_set_data_full(G_OBJECT(src), "row-data", new Song(song), [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<StreamingSearchView *>(data);
                     auto *dragged = static_cast<Song *>(g_object_get_data(G_OBJECT(s), "row-data"));
                     SongList songs = dragged ? SongList{*dragged} : SongList{};
                     for (const Song &selected : self->SelectedSongs()) {
                       if (dragged && selected.url() == dragged->url()) {
                         songs = self->SelectedSongs();
                         break;
                       }
                     }
                     const std::string payload = StreamingDrag::DragPayload(songs);
                     if (payload.empty()) {
                       return nullptr;
                     }
                     GValue v = G_VALUE_INIT;
                     g_value_init(&v, G_TYPE_STRING);
                     g_value_set_string(&v, payload.c_str());
                     GdkContentProvider *provider = gdk_content_provider_new_for_value(&v);
                     g_value_unset(&v);
                     return provider;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(src));
}

void StreamingSearchView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean StreamingSearchView::OnKeyPressed(guint keyval) {
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if (action == ListBoxKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::MoveUp || action == ListBoxKeyboard::Action::MoveDown || action == ListBoxKeyboard::Action::Home ||
      action == ListBoxKeyboard::Action::End) {
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(ListBoxKeyboardGtk::SelectedIndex(list_),
                                                                      ListBoxKeyboardGtk::Count(list_), action));
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Escape) {
    ResetTypeAhead();
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch && g_unichar_isprint(ch)) {
    gchar utf8[8] = {};
    typeahead_.append(utf8, static_cast<size_t>(g_unichar_to_utf8(ch, utf8)));
    if (typeahead_timeout_) {
      g_source_remove(typeahead_timeout_);
    }
    typeahead_timeout_ = g_timeout_add(1000, [](gpointer data) -> gboolean {
      auto *self = static_cast<StreamingSearchView *>(data);
      self->typeahead_timeout_ = 0;
      self->typeahead_.clear();
      return G_SOURCE_REMOVE;
    }, this);
    const int index = ListBoxKeyboard::FirstPrefixIndex(ListBoxKeyboardGtk::Labels(list_), typeahead_);
    if (index >= 0) {
      ListBoxKeyboardGtk::SelectIndex(list_, index);
    }
    return TRUE;
  }
  return FALSE;
}

SongList StreamingSearchView::SelectedSongs() const {
  SongList songs;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
        if (song) {
          static_cast<SongList *>(data)->push_back(*song);
        }
      },
      &songs);
  return songs;
}
