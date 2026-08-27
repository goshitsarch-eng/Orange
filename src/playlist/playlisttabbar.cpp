#include "playlist/playlisttabbar.h"

#include "playlist/playlisttabbarvisibility.h"

#include "translations/translations.h"

#include <algorithm>
#include <cstring>

PlaylistTabBar::PlaylistTabBar() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_overflow(widget_, GTK_OVERFLOW_HIDDEN);
  popover_ = gtk_popover_new();
  gtk_widget_set_parent(popover_, widget_);
  gtk_popover_set_has_arrow(GTK_POPOVER(popover_), FALSE);
  gtk_popover_set_autohide(GTK_POPOVER(popover_), TRUE);

  gtk_widget_set_focusable(widget_, TRUE);
  GtkGesture *right = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(right), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(right));
  g_signal_connect(right, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistTabBar *>(data);
                     self->ShowContextMenu(self->IndexAt(x, y), x, y);
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget_, keys);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return static_cast<PlaylistTabBar *>(data)->OnKeyPressed(keyval, state);
                   })),
                   this);

  GtkGesture *middle = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(middle), GDK_BUTTON_MIDDLE);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(middle));
  g_signal_connect(middle, "released", G_CALLBACK(+[](GtkGestureClick *, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistTabBar *>(data);
                     if (PlaylistTabMenu::FromRelease(self->IndexAt(x, y), 2) == PlaylistTabMenu::Click::Close) {
                       const int id = self->TabIdAt(self->IndexAt(x, y));
                       if (id >= 0) {
                         self->RequestClose(id);
                       }
                     }
                   }),
                   this);

  GtkGesture *left = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(left), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(left));
  g_signal_connect(left, "pressed", G_CALLBACK(+[](GtkGestureClick *, gint n_press, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistTabBar *>(data);
                     const char *part = self->PartAt(x, y);
                     if (part && (std::strcmp(part, "favorite") == 0 || std::strcmp(part, "close") == 0)) {
                       return;
                     }
                     const PlaylistTabMenu::Click click = PlaylistTabMenu::FromPress(self->IndexAt(x, y), 1, n_press);
                     if (click == PlaylistTabMenu::Click::New && self->new_) {
                       self->new_();
                     } else if (click == PlaylistTabMenu::Click::Rename) {
                       self->StartInlineRename(self->IndexAt(x, y));
                     }
                   }),
                   this);

  GtkDropTarget *bar_target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_drop_target_set_actions(bar_target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(bar_target));
  g_signal_connect(bar_target, "leave", G_CALLBACK(+[](GtkDropTarget *, gpointer data) {
                     static_cast<PlaylistTabBar *>(data)->CancelDragHover();
                   }),
                   this);
  g_signal_connect(bar_target, "drop",
                   G_CALLBACK((+[](GtkDropTarget *, const GValue *value, gdouble x, gdouble y, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistTabBar *>(data);
                     if (!G_VALUE_HOLDS_STRING(value)) {
                       return FALSE;
                     }
                     const char *text = g_value_get_string(value);
                     const std::string payload = text ? text : "";
                     const int index = self->IndexAt(x, y);
                     if (index < 0 && PlaylistTabMenu::DropOnEmptyCreatesPlaylist() && self->new_ &&
                         !PlaylistTabMenu::IsTabPayload(payload)) {
                       self->new_();
                     }
                     if (self->drop_) {
                       self->drop_(index < 0 ? -1 : self->TabIdAt(index), payload);
                     }
                     self->CancelDragHover();
                     return TRUE;
                   })),
                   this);
}

PlaylistTabBar::~PlaylistTabBar() {
  StopVisibilityAnim();
  HideEditor();
  CancelDragHover();
  if (popover_) {
    gtk_widget_unparent(popover_);
    popover_ = nullptr;
  }
}

void PlaylistTabBar::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void PlaylistTabBar::SetFavoriteCallback(FavoriteCallback callback) { favorite_ = std::move(callback); }

int PlaylistTabBar::CurrentIndex() const {
  int index = 0;
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!g_object_get_data(G_OBJECT(child), "playlist-id")) {
      continue;
    }
    for (GtkWidget *part = gtk_widget_get_first_child(child); part; part = gtk_widget_get_next_sibling(part)) {
      if (GTK_IS_TOGGLE_BUTTON(part) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(part))) {
        return index;
      }
    }
    ++index;
  }
  return -1;
}

gboolean PlaylistTabBar::OnKeyPressed(guint keyval, GdkModifierType state) {
  if (!PlaylistTabMenu::IsKeyboardTrigger(keyval, static_cast<unsigned>(state))) {
    return FALSE;
  }
  if (PlaylistTabMenu::ShouldShowMenu()) {
    ShowContextMenu(PlaylistTabMenu::IndexForKeyboard(CurrentIndex()), 0, PlaylistTabMenu::kKeyboardY);
  }
  return TRUE;
}

int PlaylistTabBar::TabCount() const {
  int count = 0;
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    if (g_object_get_data(G_OBJECT(child), "playlist-id")) {
      ++count;
    }
  }
  return count;
}

GtkWidget *PlaylistTabBar::TabWidget(int index) const {
  int i = 0;
  for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!g_object_get_data(G_OBJECT(child), "playlist-id")) {
      continue;
    }
    if (i == index) {
      return child;
    }
    ++i;
  }
  return nullptr;
}

int PlaylistTabBar::TabIdAt(int index) const {
  GtkWidget *tab = TabWidget(index);
  if (!tab) {
    return -1;
  }
  return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tab), "playlist-id")) - 1;
}

std::string PlaylistTabBar::TabNameAt(int index) const {
  GtkWidget *tab = TabWidget(index);
  if (!tab) {
    return {};
  }
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(tab), "playlist-name"));
  return name ? name : std::string();
}

std::vector<int> PlaylistTabBar::TabIds() const {
  std::vector<int> ids;
  for (int i = 0; i < TabCount(); ++i) {
    ids.push_back(TabIdAt(i));
  }
  return ids;
}

int PlaylistTabBar::IndexOfWidget(GtkWidget *widget) const {
  while (widget && widget != widget_) {
    if (g_object_get_data(G_OBJECT(widget), "playlist-id")) {
      int i = 0;
      for (GtkWidget *child = gtk_widget_get_first_child(widget_); child; child = gtk_widget_get_next_sibling(child)) {
        if (!g_object_get_data(G_OBJECT(child), "playlist-id")) {
          continue;
        }
        if (child == widget) {
          return i;
        }
        ++i;
      }
      return -1;
    }
    widget = gtk_widget_get_parent(widget);
  }
  return -1;
}

int PlaylistTabBar::IndexAt(double x, double y) const {
  if (GtkWidget *picked = gtk_widget_pick(widget_, x, y, GTK_PICK_DEFAULT)) {
    return IndexOfWidget(picked);
  }
  return -1;
}

const char *PlaylistTabBar::PartAt(double x, double y) const {
  GtkWidget *picked = gtk_widget_pick(widget_, x, y, GTK_PICK_DEFAULT);
  while (picked && picked != widget_) {
    if (const char *part = static_cast<const char *>(g_object_get_data(G_OBJECT(picked), "tab-part"))) {
      return part;
    }
    picked = gtk_widget_get_parent(picked);
  }
  return nullptr;
}

void PlaylistTabBar::ShowContextMenu(int index, double x, double y) {
  HideEditor();
  menu_index_ = index;
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  const int count = TabCount();
  for (const PlaylistTabMenu::Item &item : PlaylistTabMenu::Items()) {
    if (item.separator_before) {
      gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    }
    GtkWidget *button = gtk_button_new_with_label(Translations::CStr(item.label));
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_sensitive(button, PlaylistTabMenu::ActionEnabled(item.action, index, count));
    g_object_set_data(G_OBJECT(button), "action", GINT_TO_POINTER(static_cast<int>(item.action) + 1));
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistTabBar *>(data);
                       const int action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "action")) - 1;
                       gtk_popover_popdown(GTK_POPOVER(self->popover_));
                       self->ActivateAction(static_cast<PlaylistTabMenu::Action>(action), self->menu_index_);
                     }),
                     this);
    gtk_box_append(GTK_BOX(box), button);
  }
  gtk_popover_set_child(GTK_POPOVER(popover_), box);
  GdkRectangle rect;
  rect.x = static_cast<int>(x);
  rect.y = static_cast<int>(y);
  rect.width = 1;
  rect.height = 1;
  if (PlaylistTabMenu::IsKeyboardAnchor(y)) {
    GtkWidget *tab = TabWidget(index >= 0 ? index : 0);
    rect.x = 0;
    rect.y = 0;
    rect.width = tab ? std::max(1, gtk_widget_get_width(tab)) : 1;
    rect.height = tab ? std::max(1, gtk_widget_get_height(tab)) : 1;
  }
  gtk_popover_set_pointing_to(GTK_POPOVER(popover_), &rect);
  gtk_popover_popup(GTK_POPOVER(popover_));
}

void PlaylistTabBar::ActivateAction(PlaylistTabMenu::Action action, int index) {
  const int id = TabIdAt(index);
  switch (action) {
    case PlaylistTabMenu::Action::Star:
      if (index >= 0 && index < static_cast<int>(favorites_.size())) {
        favorites_[static_cast<size_t>(index)]->SetFavorite(PlaylistTabMenu::ToggledFavorite(favorites_[static_cast<size_t>(index)]->IsFavorite()));
      }
      break;
    case PlaylistTabMenu::Action::Close:
      if (id >= 0) {
        RequestClose(id);
      }
      break;
    case PlaylistTabMenu::Action::Rename:
      if (id >= 0 && rename_) {
        rename_(id, {});
      }
      break;
    case PlaylistTabMenu::Action::Save:
      if (id >= 0 && save_) {
        save_(id);
      }
      break;
    case PlaylistTabMenu::Action::New:
      if (new_) {
        new_();
      }
      break;
    case PlaylistTabMenu::Action::Load:
      if (load_) {
        load_();
      }
      break;
  }
}

void PlaylistTabBar::StartInlineRename(int index) {
  HideEditor();
  GtkWidget *tab = TabWidget(index);
  if (!tab) {
    return;
  }
  GtkWidget *button = nullptr;
  for (GtkWidget *child = gtk_widget_get_first_child(tab); child; child = gtk_widget_get_next_sibling(child)) {
    if (const char *part = static_cast<const char *>(g_object_get_data(G_OBJECT(child), "tab-part")); part && std::strcmp(part, "name") == 0) {
      button = child;
      break;
    }
  }
  if (!button) {
    return;
  }
  rename_id_ = TabIdAt(index);
  rename_button_ = button;
  gtk_widget_set_visible(button, FALSE);
  rename_entry_ = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(rename_entry_), TabNameAt(index).c_str());
  gtk_box_insert_child_after(GTK_BOX(tab), rename_entry_, button);
  gtk_widget_grab_focus(rename_entry_);
  g_signal_connect(rename_entry_, "activate", G_CALLBACK(+[](GtkEntry *, gpointer data) { static_cast<PlaylistTabBar *>(data)->ApplyInlineRename(); }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(rename_entry_, keys);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     if (keyval == GDK_KEY_Escape) {
                       static_cast<PlaylistTabBar *>(data)->HideEditor();
                       return TRUE;
                     }
                     return FALSE;
                   })),
                   this);
  GtkEventController *focus = gtk_event_controller_focus_new();
  gtk_widget_add_controller(rename_entry_, focus);
  g_signal_connect(focus, "leave", G_CALLBACK(+[](GtkEventControllerFocus *, gpointer data) {
                     if (PlaylistTabMenu::ShouldCommitRenameOnFocusLoss()) {
                       static_cast<PlaylistTabBar *>(data)->ApplyInlineRename();
                       return;
                     }
                     static_cast<PlaylistTabBar *>(data)->HideEditor();
                   }),
                   this);
}

void PlaylistTabBar::ApplyInlineRename() {
  if (!rename_entry_) {
    return;
  }
  const std::string name = gtk_editable_get_text(GTK_EDITABLE(rename_entry_));
  const int id = rename_id_;
  const std::string old_name = rename_button_ ? TabNameAt(IndexOfWidget(rename_button_)) : std::string();
  HideEditor();
  if (id >= 0 && rename_ && PlaylistTabMenu::ShouldApplyRename(old_name, name)) {
    rename_(id, name);
  }
}

void PlaylistTabBar::HideEditor() {
  if (rename_entry_) {
    GtkWidget *parent = gtk_widget_get_parent(rename_entry_);
    if (parent) {
      gtk_box_remove(GTK_BOX(parent), rename_entry_);
    }
    rename_entry_ = nullptr;
  }
  if (rename_button_) {
    gtk_widget_set_visible(rename_button_, TRUE);
    rename_button_ = nullptr;
  }
  rename_id_ = -1;
}

void PlaylistTabBar::RequestClose(int id) {
  if (PlaylistTabMenu::ShouldDiscardRenameBeforeClose(rename_entry_ != nullptr)) {
    HideEditor();
  }
  if (id >= 0 && close_) {
    close_(id);
  }
}

void PlaylistTabBar::StartDragHover(int index) {
  if (index < 0 || index == hover_index_) {
    if (index < 0) {
      CancelDragHover();
    }
    return;
  }
  CancelDragHover();
  hover_index_ = index;
  hover_timeout_ = g_timeout_add(PlaylistTabMenu::kDragHoverTimeoutMs, [](gpointer data) -> gboolean {
    auto *self = static_cast<PlaylistTabBar *>(data);
    self->hover_timeout_ = 0;
    const int hovered = self->hover_index_;
    self->hover_index_ = -1;
    const std::string name = self->TabNameAt(hovered);
    if (!name.empty() && self->changed_) {
      self->changed_(name);
    }
    return G_SOURCE_REMOVE;
  }, this);
}

void PlaylistTabBar::CancelDragHover() {
  hover_index_ = -1;
  if (hover_timeout_) {
    g_source_remove(hover_timeout_);
    hover_timeout_ = 0;
  }
}

void PlaylistTabBar::SetupTab(GtkWidget *tab, int index, int id, const std::string &name) {
  g_object_set_data(G_OBJECT(tab), "playlist-id", GINT_TO_POINTER(id + 1));
  g_object_set_data_full(G_OBJECT(tab), "playlist-name", g_strdup(name.c_str()), g_free);

  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_MOVE);
  g_object_set_data(G_OBJECT(src), "tab-id", GINT_TO_POINTER(id + 1));
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *, double, double, gpointer data) -> GdkContentProvider * {
                     const int tab_id = GPOINTER_TO_INT(data) - 1;
                     const std::string payload = PlaylistTabMenu::TabPayload(tab_id);
                     GValue v = G_VALUE_INIT;
                     g_value_init(&v, G_TYPE_STRING);
                     g_value_set_string(&v, payload.c_str());
                     GdkContentProvider *provider = gdk_content_provider_new_for_value(&v);
                     g_value_unset(&v);
                     return provider;
                   })),
                   GINT_TO_POINTER(id + 1));
  gtk_widget_add_controller(tab, GTK_EVENT_CONTROLLER(src));

  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  g_object_set_data(G_OBJECT(target), "tab-index", GINT_TO_POINTER(index + 1));
  gtk_widget_add_controller(tab, GTK_EVENT_CONTROLLER(target));
  g_signal_connect(target, "enter", G_CALLBACK((+[](GtkDropTarget *drop, gdouble, gdouble, gpointer data) -> GdkDragAction {
                     auto *self = static_cast<PlaylistTabBar *>(data);
                     self->StartDragHover(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(drop), "tab-index")) - 1);
                     return GDK_ACTION_COPY;
                   })),
                   this);
  g_signal_connect(target, "leave", G_CALLBACK(+[](GtkDropTarget *, gpointer data) { static_cast<PlaylistTabBar *>(data)->CancelDragHover(); }), this);
  g_signal_connect(target, "drop",
                   G_CALLBACK((+[](GtkDropTarget *drop, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistTabBar *>(data);
                     if (!G_VALUE_HOLDS_STRING(value)) {
                       return FALSE;
                     }
                     const char *text = g_value_get_string(value);
                     const std::string payload = text ? text : "";
                     const int dest = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(drop), "tab-index")) - 1;
                     if (PlaylistTabMenu::IsTabPayload(payload) && self->reorder_) {
                       int from = -1;
                       const int dragged = PlaylistTabMenu::ParseTabId(payload);
                       const std::vector<int> ids = self->TabIds();
                       for (size_t i = 0; i < ids.size(); ++i) {
                         if (ids[i] == dragged) {
                           from = static_cast<int>(i);
                           break;
                         }
                       }
                       self->reorder_(PlaylistTabMenu::ReorderIds(ids, from, dest));
                       self->CancelDragHover();
                       return TRUE;
                     }
                     if (self->drop_) {
                       self->drop_(self->TabIdAt(dest), payload);
                     }
                     self->CancelDragHover();
                     return TRUE;
                   })),
                   this);
}

void PlaylistTabBar::Refresh(PlaylistManager *manager, const std::string &active_name, PlaylistListLook::Playback playback) {
  HideEditor();
  CancelDragHover();
  favorites_.clear();
  GtkWidget *child = gtk_widget_get_first_child(widget_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    if (child != popover_) {
      gtk_widget_unparent(child);
    }
    child = next;
  }
  if (!manager) {
    return;
  }
  int index = 0;
  for (const auto &playlist : manager->playlists()) {
    GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    if (const char *icon_name = PlaylistListLook::PlaybackIconName(PlaylistListLook::IsActiveName(playlist->name(), active_name), playback)) {
      gtk_box_append(GTK_BOX(tab), gtk_image_new_from_icon_name(icon_name));
    }
    GtkWidget *button = gtk_toggle_button_new_with_label(playlist->name().c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), playlist.get() == manager->current());
    g_object_set_data(G_OBJECT(button), "tab-part", const_cast<char *>("name"));
    g_object_set_data_full(G_OBJECT(button), "playlist-name", g_strdup(playlist->name().c_str()), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistTabBar *>(data);
                       const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "playlist-name"));
                       if (name && self->changed_) {
                         self->changed_(name);
                       }
                     }),
                     this);
    auto favorite = std::make_unique<FavoriteWidget>(playlist->id(), playlist->favorite());
    favorite->SetChangedCallback([this, name = playlist->name()](int, bool on) {
      if (favorite_) {
        favorite_(name, on);
      }
    });
    GtkWidget *close = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(close, "flat");
    gtk_widget_add_css_class(close, "circular");
    gtk_widget_set_tooltip_text(close, Translations::CStr("Close playlist"));
    g_object_set_data(G_OBJECT(close), "tab-part", const_cast<char *>("close"));
    g_object_set_data(G_OBJECT(close), "playlist-id", GINT_TO_POINTER(playlist->id() + 1));
    g_signal_connect(close, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistTabBar *>(data);
                       const int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "playlist-id")) - 1;
                       self->RequestClose(id);
                     }),
                     this);
    gtk_box_append(GTK_BOX(tab), button);
    gtk_box_append(GTK_BOX(tab), favorite->widget());
    gtk_box_append(GTK_BOX(tab), close);
    SetupTab(tab, index, playlist->id(), playlist->name());
    gtk_box_append(GTK_BOX(widget_), tab);
    favorites_.push_back(std::move(favorite));
    ++index;
  }
  UpdateVisibility(PlaylistTabBarVisibility::ShouldShow(index));
}

void PlaylistTabBar::StopVisibilityAnim() {
  if (anim_id_) {
    g_source_remove(anim_id_);
    anim_id_ = 0;
  }
}

void PlaylistTabBar::UpdateVisibility(bool show) {
  if (show == shown_ && anim_id_ == 0) {
    gtk_widget_set_visible(widget_, show ? TRUE : FALSE);
    gtk_widget_set_size_request(widget_, -1, -1);
    return;
  }
  if (show == shown_ && anim_id_ != 0 && anim_showing_ == show) {
    return;
  }
  StopVisibilityAnim();
  gtk_widget_set_visible(widget_, TRUE);
  gtk_widget_set_overflow(widget_, GTK_OVERFLOW_HIDDEN);
  int natural = gtk_widget_get_height(widget_);
  if (natural <= 0) {
    int minimum = 0;
    int nat = 0;
    gtk_widget_measure(widget_, GTK_ORIENTATION_VERTICAL, -1, &minimum, &nat, nullptr, nullptr);
    natural = nat > 0 ? nat : 36;
  }
  anim_natural_ = natural;
  anim_showing_ = show;
  anim_start_us_ = g_get_monotonic_time();
  gtk_widget_set_size_request(widget_, -1, PlaylistTabBarVisibility::HeightAt(0, natural, show));
  anim_id_ = g_timeout_add(
      16,
      +[](gpointer data) -> gboolean {
        static_cast<PlaylistTabBar *>(data)->TickVisibility();
        return G_SOURCE_CONTINUE;
      },
      this);
}

void PlaylistTabBar::TickVisibility() {
  const int elapsed = static_cast<int>((g_get_monotonic_time() - anim_start_us_) / 1000);
  gtk_widget_set_size_request(widget_, -1, PlaylistTabBarVisibility::HeightAt(elapsed, anim_natural_, anim_showing_));
  if (elapsed < PlaylistTabBarVisibility::kAnimationMs) {
    return;
  }
  StopVisibilityAnim();
  shown_ = anim_showing_;
  gtk_widget_set_visible(widget_, shown_ ? TRUE : FALSE);
  gtk_widget_set_size_request(widget_, -1, -1);
}
