#include "streaming/streamingcollectionview.h"

#include "collection/collectionempty.h"
#include "collection/collectionsearchlabels.h"
#include "filterparser/filterparser.h"
#include "collection/collectionfiltermenu.h"
#include "collection/collectionitemdelegate.h"
#include "collection/collectiontree.h"
#include "collection/groupbydialog.h"
#include "dialogs/dialoghelpers.h"
#include "streaming/streamingcollectionactions.h"
#include "streaming/streamingcollectionfilter.h"
#include "streaming/streamingcollectionlabels.h"
#include "streaming/streamingcollectiontree.h"
#include "streaming/streamingcover.h"
#include "streaming/streamingdrag.h"
#include "streaming/streamingsearchgroup.h"
#include "streaming/streamingsearchopts.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <vector>
#include "widgets/filtersearchkeyboard.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"
#include "widgets/listboxtreepressgtk.h"

StreamingCollectionView::StreamingCollectionView(const std::string &title) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(header, 8);
  gtk_widget_set_margin_end(header, 8);
  gtk_widget_set_margin_top(header, 6);
  gtk_widget_set_margin_bottom(header, 4);
  GtkWidget *label = gtk_label_new(title.c_str());
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  back_ = gtk_button_new_from_icon_name("go-previous-symbolic");
  gtk_widget_set_tooltip_text(back_, Translations::CStr(StreamingCollectionLabels::Back()));
  gtk_widget_set_visible(back_, FALSE);
  g_signal_connect(back_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<StreamingCollectionView *>(data)->PopBrowse(); }),
                   this);
  GtkWidget *refresh = gtk_button_new_from_icon_name("view-refresh-symbolic");
  gtk_widget_set_tooltip_text(refresh, Translations::CStr(StreamingCollectionLabels::Refresh()));
  g_signal_connect(refresh, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     if (self->refresh_) {
                       self->refresh_();
                     }
                   }),
                   this);
  grouping_ = CollectionGrouping::LoadCurrent();
  pretty_covers_btn_ = gtk_check_button_new_with_label(Translations::CStr("Pretty covers"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(pretty_covers_btn_), pretty_covers_);
  g_signal_connect(pretty_covers_btn_, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     self->pretty_covers_ = gtk_check_button_get_active(button);
                     self->PersistPrettyCovers();
                     self->Rebuild();
                   }),
                   this);
  group_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(group_button_), "view-list-symbolic");
  gtk_widget_set_tooltip_text(group_button_, Translations::CStr("Group by"));
  BuildGroupMenu();
  gtk_box_append(GTK_BOX(header), back_);
  gtk_box_append(GTK_BOX(header), label);
  gtk_box_append(GTK_BOX(header), pretty_covers_btn_);
  gtk_box_append(GTK_BOX(header), group_button_);
  gtk_box_append(GTK_BOX(header), refresh);
  filter_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(filter_entry_), Translations::CStr(CollectionSearchLabels::Placeholder()));
  gtk_widget_set_tooltip_text(filter_entry_, Translations::CStr(FilterParser::ToolTip().c_str()));
  gtk_widget_set_margin_start(filter_entry_, 8);
  gtk_widget_set_margin_end(filter_entry_, 8);
  g_signal_connect(filter_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
                     self->SetFilter(text ? text : "");
                   }),
                   this);
  GtkEventController *filter_keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(filter_entry_, filter_keys);
  g_signal_connect(filter_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     const FilterSearchKeyboard::Action action = FilterSearchKeyboard::FromSearchKey(keyval);
                     if (action == FilterSearchKeyboard::Action::MoveUp || action == FilterSearchKeyboard::Action::MoveDown) {
                       self->FocusListAndMove(keyval);
                       return TRUE;
                     }
                     if (action == FilterSearchKeyboard::Action::Clear) {
                       gtk_editable_set_text(GTK_EDITABLE(self->filter_entry_), "");
                       return TRUE;
                     }
                     return FALSE;
                   })),
                   this);
  status_label_ = gtk_label_new("");
  gtk_widget_set_margin_start(status_label_, 8);
  gtk_widget_set_halign(status_label_, GTK_ALIGN_START);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  ListBoxTreePressGtk::Attach(list_, this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "empty-streaming")) && self->refresh_) {
                       self->refresh_();
                       return;
                     }
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
                     if (song) {
                       self->ActivateSong(*song);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed",
                   G_CALLBACK((+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y));
                     const bool empty = row && GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "empty-streaming"));
                     if (self->menu_ && StreamingCollectionActions::ShouldShowContextMenu(row && !empty)) {
                       self->menu_(self->SelectedSongs());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   })),
                   this);
  gtk_box_append(GTK_BOX(widget_), header);
  gtk_box_append(GTK_BOX(widget_), filter_entry_);
  gtk_box_append(GTK_BOX(widget_), status_label_);
  gtk_box_append(GTK_BOX(widget_), scroll);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<StreamingCollectionView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

StreamingCollectionView::~StreamingCollectionView() {
  if (alive_) {
    *alive_ = false;
  }
  ResetTypeAhead();
}

void StreamingCollectionView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void StreamingCollectionView::SetEnqueueCallback(EnqueueCallback callback) { enqueue_ = std::move(callback); }

void StreamingCollectionView::HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state) {
  const CollectionTreeClick::Action action = CollectionTreeClick::FromPress(button, n_press, state);
  GtkListBoxRow *row = ListBoxTreePressGtk::RowAtY(list_, y);
  if (action == CollectionTreeClick::Action::Enqueue) {
    if (row && CollectionTreeClick::SelectRowBeforeEnqueue(gtk_list_box_row_is_selected(row))) {
      ListBoxTreePressGtk::SelectRowIfNeeded(list_, row);
    }
    if (enqueue_) {
      enqueue_(SelectedSongs());
    }
    return;
  }
  if (action != CollectionTreeClick::Action::ToggleExpand || !row || ListBoxTreePressGtk::OnExpandControl(list_, x, y)) {
    return;
  }
  auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
  if (CollectionTreeClick::ShouldToggleFromRowClick(false, CollectionTree::IsExpandable(item))) {
    ToggleExpanded(item);
  }
}

void StreamingCollectionView::SetRefreshCallback(RefreshCallback callback) { refresh_ = std::move(callback); }

void StreamingCollectionView::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void StreamingCollectionView::SetService(StreamingService *service) {
  service_ = service;
  pretty_covers_ = StreamingCover::LoadPrettyCovers(service_ ? service_->name() : std::string());
  if (pretty_covers_btn_) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(pretty_covers_btn_), pretty_covers_);
  }
  Rebuild();
}

void StreamingCollectionView::PersistPrettyCovers() {
  StreamingCover::SavePrettyCovers(service_ ? service_->name() : std::string(), pretty_covers_);
}

void StreamingCollectionView::SetGrouping(const CollectionGrouping::Grouping &grouping) {
  if (grouping_ == grouping) {
    return;
  }
  grouping_ = grouping;
  Rebuild();
}

void StreamingCollectionView::ApplyGrouping(const CollectionGrouping::Grouping &grouping) {
  grouping_ = grouping;
  CollectionGrouping::SaveCurrent(grouping);
  if (grouping_changed_) {
    grouping_changed_(grouping_);
  }
  Rebuild();
}

void StreamingCollectionView::SetFilter(const std::string &filter) {
  if (!StreamingCollectionFilter::TextSearchEnabled(filter_options_)) {
    return;
  }
  filter_ = filter;
  Rebuild();
}

void StreamingCollectionView::SetFilterOptions(const CollectionFilterOptions &options) {
  filter_options_ = options;
  if (!StreamingCollectionFilter::TextSearchEnabled(filter_options_)) {
    filter_.clear();
    if (filter_entry_) {
      gtk_editable_set_text(GTK_EDITABLE(filter_entry_), "");
    }
  }
  if (filter_entry_) {
    gtk_widget_set_sensitive(filter_entry_, StreamingCollectionFilter::TextSearchEnabled(filter_options_));
  }
  Rebuild();
}

void StreamingCollectionView::SetStatus(const std::string &status) { gtk_label_set_text(GTK_LABEL(status_label_), status.c_str()); }

void StreamingCollectionView::SetSongs(const SongList &songs) {
  stack_.clear();
  songs_ = songs;
  Rebuild();
  UpdateBack();
}

void StreamingCollectionView::PushSongs(const SongList &songs) {
  stack_.push_back({songs_, status_label_ ? gtk_label_get_text(GTK_LABEL(status_label_)) : ""});
  songs_ = songs;
  Rebuild();
  UpdateBack();
}

void StreamingCollectionView::PopBrowse() {
  if (stack_.empty()) {
    return;
  }
  songs_ = stack_.back().songs;
  const std::string status = stack_.back().status;
  stack_.pop_back();
  Rebuild();
  if (status_label_) {
    gtk_label_set_text(GTK_LABEL(status_label_), status.c_str());
  }
  UpdateBack();
}

void StreamingCollectionView::UpdateBack() {
  if (back_) {
    gtk_widget_set_visible(back_, !stack_.empty());
  }
}

void StreamingCollectionView::ActivateSong(const Song &song) {
  if (activate_) {
    activate_(song);
  }
}

SongList StreamingCollectionView::Visible() const { return StreamingCollectionFilter::Apply(songs_, filter_options_, filter_); }

void StreamingCollectionView::ToggleExpanded(const CollectionItem *item) {
  if (CollectionTree::Toggle(&expanded_, item) || CollectionTree::IsExpandable(item)) {
    Rebuild(false);
  }
}

void StreamingCollectionView::AppendItem(const CollectionItem *item, int depth, bool filter_active) {
  if (!item) {
    return;
  }
  const bool expandable = CollectionTree::IsExpandable(item);
  const bool expanded = CollectionTree::ShowChildren(item, filter_active, expanded_);
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(row_box, 8 + depth * 12);
  gtk_widget_set_margin_end(row_box, 8);
  gtk_widget_set_margin_top(row_box, 4);
  gtk_widget_set_margin_bottom(row_box, 4);
  if (expandable) {
    GtkWidget *toggle = gtk_button_new_from_icon_name(expanded ? "pan-down-symbolic" : "pan-end-symbolic");
    gtk_widget_add_css_class(toggle, "flat");
    gtk_widget_add_css_class(toggle, "circular");
    gtk_widget_set_tooltip_text(toggle, expanded ? Translations::CStr("Collapse") : Translations::CStr("Expand"));
    g_object_set_data(G_OBJECT(toggle), "item", const_cast<CollectionItem *>(item));
    g_signal_connect(toggle, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                       auto *self = static_cast<StreamingCollectionView *>(data);
                       self->ToggleExpanded(static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(button), "item")));
                     }),
                     this);
    gtk_box_append(GTK_BOX(row_box), toggle);
  }
  const Song cover_song = StreamingCollectionTree::RepresentativeSong(item);
  if (StreamingCover::ShouldShowThumb(pretty_covers_) && (!cover_song.url().empty() || !StreamingCover::CoverUrl(cover_song).empty())) {
    GtkWidget *image = gtk_image_new_from_icon_name(StreamingCover::kPlaceholderIcon);
    gtk_image_set_pixel_size(GTK_IMAGE(image), StreamingCover::kArtHeight);
    gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row_box), image);
    LoadCover(image, cover_song);
  }
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(box, TRUE);
  GtkWidget *primary = gtk_label_new(CollectionItemDelegate::PrimaryText(item).c_str());
  gtk_widget_set_halign(primary, GTK_ALIGN_START);
  if (expandable) {
    gtk_widget_add_css_class(primary, "heading");
  }
  gtk_box_append(GTK_BOX(box), primary);
  const std::string secondary = CollectionItemDelegate::SecondaryText(item);
  if (!secondary.empty()) {
    GtkWidget *sub = gtk_label_new(secondary.c_str());
    gtk_widget_add_css_class(sub, "dim-label");
    gtk_widget_set_halign(sub, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), sub);
  }
  gtk_box_append(GTK_BOX(row_box), box);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
  g_object_set_data(G_OBJECT(row), "item", const_cast<CollectionItem *>(item));
  auto *copy = new Song(cover_song);
  g_object_set_data_full(G_OBJECT(row), "row-data", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  SetupRowDrag(row, cover_song);
  gtk_list_box_append(GTK_LIST_BOX(list_), row);
  if (expanded) {
    for (const auto &child : item->children) {
      AppendItem(child.get(), depth + 1, filter_active);
    }
  }
}

void StreamingCollectionView::SaveFocus() { CollectionFocus::Capture(SelectedItem(), &focus_); }

const CollectionItem *StreamingCollectionView::SelectedItem() const {
  const CollectionItem *item = nullptr;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto **out = static_cast<const CollectionItem **>(data);
        if (*out) {
          return;
        }
        *out = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
      },
      &item);
  return item;
}

void StreamingCollectionView::RestoreFocus() {
  if (!CollectionFocus::ShouldRestore(focus_)) {
    return;
  }
  const std::set<std::string> needed = CollectionFocus::ExpandKeys(model_.root(), focus_);
  if (CollectionFocus::NeedsExpand(expanded_, needed)) {
    CollectionFocus::MergeExpand(&expanded_, needed);
    Rebuild(false);
  }
  SelectFocusItem();
}

void StreamingCollectionView::SelectFocusItem() {
  const CollectionItem *target = CollectionFocus::FindTarget(model_.root(), focus_);
  if (!target) {
    return;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(list_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(child), "item"));
    if (item != target) {
      continue;
    }
    gtk_list_box_unselect_all(GTK_LIST_BOX(list_));
    gtk_list_box_select_row(GTK_LIST_BOX(list_), GTK_LIST_BOX_ROW(child));
    gtk_widget_grab_focus(child);
    return;
  }
}

void StreamingCollectionView::Rebuild(bool preserve_focus) {
  if (preserve_focus) {
    SaveFocus();
  }
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  const SongList visible = Visible();
  const bool filter_active = StreamingCollectionTree::FilterActive(filter_);
  model_.Reset(visible, grouping_, CollectionGrouping::SeparateAlbumsByGrouping(), false, false);
  if (visible.empty()) {
    GtkWidget *row = gtk_list_box_row_new();
    const bool collection_empty = CollectionEmpty::IsEmptyCollection(static_cast<int>(songs_.size()));
    if (collection_empty) {
      GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
      gtk_widget_set_margin_top(box, 24);
      gtk_widget_set_margin_bottom(box, 24);
      gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
      GtkWidget *image = gtk_image_new_from_resource(CollectionEmpty::kResourcePath);
      gtk_image_set_pixel_size(GTK_IMAGE(image), 75);
      gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
      GtkWidget *title = gtk_label_new(Translations::CStr(CollectionEmpty::StreamingTitle()));
      gtk_widget_add_css_class(title, "heading");
      gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
      GtkWidget *hint = gtk_label_new(Translations::CStr(CollectionEmpty::StreamingHint()));
      gtk_widget_add_css_class(hint, "dim-label");
      gtk_widget_set_halign(hint, GTK_ALIGN_CENTER);
      gtk_box_append(GTK_BOX(box), image);
      gtk_box_append(GTK_BOX(box), title);
      gtk_box_append(GTK_BOX(box), hint);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
      g_object_set_data(G_OBJECT(row), "empty-streaming", GINT_TO_POINTER(1));
      gtk_widget_set_cursor_from_name(row, "pointer");
      GtkGesture *click = gtk_gesture_click_new();
      gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
      gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));
      g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble, gdouble, gpointer data) {
                         auto *self = static_cast<StreamingCollectionView *>(data);
                         if (self->refresh_) {
                           self->refresh_();
                         }
                       }),
                       this);
    } else {
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), gtk_label_new(Translations::CStr(CollectionEmpty::NoMatches())));
    }
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    SetStatus(songs_.empty() ? "0 items" : "0 shown");
    return;
  }
  ++cover_gen_;
  if (model_.root()) {
    for (const auto &node : model_.root()->children) {
      AppendItem(node.get(), 0, filter_active);
    }
  }
  SetStatus(StreamingCollectionTree::StatusText(static_cast<int>(visible.size())));
  if (preserve_focus) {
    RestoreFocus();
  }
}

void StreamingCollectionView::LoadCover(GtkWidget *image, const Song &song) {
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

void StreamingCollectionView::BuildGroupMenu() {
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
    g_snprintf(action, sizeof(action), "streamcoll.preset(%d)", static_cast<int>(i));
    g_menu_append(menu, Translations::CStr(presets[i].label), action);
  }
  g_menu_append(menu, Translations::CStr("Advanced grouping…"), "streamcoll.advanced");
  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *preset = g_simple_action_new("preset", G_VARIANT_TYPE_INT32);
  g_signal_connect(preset, "activate", G_CALLBACK(+[](GSimpleAction *, GVariant *param, gpointer data) {
                     auto *self = static_cast<StreamingCollectionView *>(data);
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
                     auto *self = static_cast<StreamingCollectionView *>(data);
                     GtkRoot *root = gtk_widget_get_root(self->widget_);
                     GtkWindow *parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
                     GroupByDialog::Show(parent, self->grouping_, [self](const CollectionGrouping::Grouping &grouping) {
                       self->ApplyGrouping(grouping);
                     });
                   }),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(advanced));
  gtk_widget_insert_action_group(group_button_, "streamcoll", G_ACTION_GROUP(group));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(group_button_), G_MENU_MODEL(menu));
  g_object_unref(group);
  g_object_unref(menu);
}

void StreamingCollectionView::SetupRowDrag(GtkWidget *row, const Song &song) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(src), "row-data", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<StreamingCollectionView *>(data);
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

SongList StreamingCollectionView::SelectedSongs() const {
  SongList songs;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
        const SongList more = StreamingCollectionTree::SongsFromItem(item);
        if (!more.empty()) {
          static_cast<SongList *>(data)->insert(static_cast<SongList *>(data)->end(), more.begin(), more.end());
          return;
        }
        auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
        if (song) {
          static_cast<SongList *>(data)->push_back(*song);
        }
      },
      &songs);
  return songs;
}

std::string StreamingCollectionView::SelectedSearchQuery() const {
  std::string query;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *query = static_cast<std::string *>(data);
        if (!query->empty()) {
          return;
        }
        auto *item = static_cast<const CollectionItem *>(g_object_get_data(G_OBJECT(row), "item"));
        Song song;
        auto *row_song = static_cast<Song *>(g_object_get_data(G_OBJECT(row), "row-data"));
        if (row_song) {
          song = *row_song;
        }
        *query = StreamingSearchOpts::QueryFromPrimary(CollectionItemDelegate::PrimaryText(item), song, StreamingService::SearchType::Songs);
      },
      &query);
  if (query.empty()) {
    const SongList songs = SelectedSongs();
    if (!songs.empty()) {
      query = StreamingSearchOpts::QueryFromSong(songs.front(), StreamingService::SearchType::Songs);
    }
  }
  return query;
}

void StreamingCollectionView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

void StreamingCollectionView::FocusFilter() {
  if (filter_entry_) {
    gtk_widget_grab_focus(filter_entry_);
  }
}

bool StreamingCollectionView::SearchFieldHasFocus() const { return filter_entry_ && gtk_widget_has_focus(filter_entry_); }

void StreamingCollectionView::FocusListAndMove(unsigned keyval) {
  gtk_widget_grab_focus(list_);
  const ListBoxKeyboard::Action move = FilterSearchKeyboard::MoveAction(FilterSearchKeyboard::FromSearchKey(keyval));
  if (move == ListBoxKeyboard::Action::MoveUp || move == ListBoxKeyboard::Action::MoveDown) {
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(ListBoxKeyboardGtk::SelectedIndex(list_),
                                                                      ListBoxKeyboardGtk::Count(list_), move));
  }
}

gboolean StreamingCollectionView::OnKeyPressed(guint keyval) {
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if ((action == ListBoxKeyboard::Action::Backspace || action == ListBoxKeyboard::Action::Escape) && CanGoBack()) {
    PopBrowse();
    return TRUE;
  }
  if (FilterSearchKeyboard::FromTreeKey(keyval) == FilterSearchKeyboard::Action::FocusFilter) {
    ResetTypeAhead();
    FocusFilter();
    return TRUE;
  }
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
      auto *self = static_cast<StreamingCollectionView *>(data);
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
