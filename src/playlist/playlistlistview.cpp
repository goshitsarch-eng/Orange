#include "playlist/playlistlistview.h"

#include "playlist/playlistlistkeyboard.h"
#include "playlist/playlistlistleft.h"
#include "translations/translations.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"
#include "widgets/listboxtreepressgtk.h"
#include "widgets/listboxensurevisible.h"

PlaylistListView::PlaylistListView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  ListBoxTreePressGtk::Attach(list_, this);
  g_signal_connect(list_, "selected-rows-changed", G_CALLBACK(+[](GtkListBox *, gpointer data) {
                     static_cast<PlaylistListView *>(data)->NotifySelectionChanged();
                   }),
                   this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<PlaylistListView *>(data);
                     const gboolean folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "playlist-folder")) == 1;
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
                     if (folder) {
                       return;
                     }
                     if (name && self->activate_) {
                       self->activate_(name);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<PlaylistListView *>(data);
                     if (GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y))) {
                       if (!gtk_list_box_row_is_selected(row)) {
                         gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                         gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                       }
                     }
                     if (self->menu_) {
                       self->menu_(self->SelectedName());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<PlaylistListView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

PlaylistListView::~PlaylistListView() {
  CancelDragHover();
  ResetTypeAhead();
}

void PlaylistListView::HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state) {
  const CollectionTreeClick::Action action = CollectionTreeClick::FromPress(button, n_press, state);
  GtkListBoxRow *row = ListBoxTreePressGtk::RowAtY(list_, y);
  if (!row) {
    return;
  }
  const bool folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "playlist-folder")) == 1;
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
  const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-path"));
  if (action == CollectionTreeClick::Action::Enqueue) {
    if (CollectionTreeClick::SelectRowBeforeEnqueue(gtk_list_box_row_is_selected(row))) {
      ListBoxTreePressGtk::SelectRowIfNeeded(list_, row);
    }
    if (!folder && name && activate_) {
      activate_(name);
    }
    return;
  }
  if (action != CollectionTreeClick::Action::ToggleExpand || ListBoxTreePressGtk::OnExpandControl(list_, x, y)) {
    return;
  }
  if (folder && path && toggle_) {
    toggle_(path);
  }
}

bool PlaylistListView::ApplyTreeLeft() {
  const bool folder = SelectedIsFolder();
  const std::string path = SelectedPath();
  if (path.empty() && !folder) {
    return false;
  }
  const bool expanded = SelectedExpanded();
  const std::string focus = PlaylistListLeft::FocusPath(folder, expanded, path);
  const std::string collapse = PlaylistListLeft::CollapsePath(folder, expanded, path);
  if (focus.empty() && collapse.empty()) {
    return false;
  }
  if (!collapse.empty() && toggle_) {
    toggle_(collapse);
  }
  SelectFolder(focus);
  return true;
}

std::string PlaylistListView::SelectedPath() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row) {
    return {};
  }
  const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-path"));
  return path ? path : "";
}

bool PlaylistListView::SelectedExpanded() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  return row && GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "playlist-expanded")) == 1;
}

void PlaylistListView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean PlaylistListView::OnKeyPressed(guint keyval) {
  const PlaylistListKeyboard::Action action = PlaylistListKeyboard::FromKey(keyval);
  if (action == PlaylistListKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == PlaylistListKeyboard::Action::Collapse && ApplyTreeLeft()) {
    return TRUE;
  }
  if (action == PlaylistListKeyboard::Action::Expand && SelectedIsFolder() && toggle_) {
    if (!SelectedExpanded()) {
      toggle_(SelectedFolderPath());
    }
    return TRUE;
  }
  if (action == PlaylistListKeyboard::Action::Delete && delete_) {
    if (SelectedIsFolder()) {
      delete_(SelectedFolderPath());
    } else if (!SelectedName().empty()) {
      delete_(SelectedName());
    }
    return TRUE;
  }
  if (action == PlaylistListKeyboard::Action::MoveUp || action == PlaylistListKeyboard::Action::MoveDown ||
      action == PlaylistListKeyboard::Action::Home || action == PlaylistListKeyboard::Action::End) {
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(ListBoxKeyboardGtk::SelectedIndex(list_),
                                                                      ListBoxKeyboardGtk::Count(list_),
                                                                      PlaylistListKeyboard::MoveAction(action)));
    return TRUE;
  }
  if (action == PlaylistListKeyboard::Action::Escape) {
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
      auto *self = static_cast<PlaylistListView *>(data);
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

void PlaylistListView::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

bool PlaylistListView::HasSelection() const { return !SelectedName().empty() || SelectedIsFolder(); }

void PlaylistListView::NotifySelectionChanged() {
  if (selection_changed_) {
    selection_changed_();
  }
}

void PlaylistListView::CancelDragHover() {
  hover_name_.clear();
  if (hover_timeout_) {
    g_source_remove(hover_timeout_);
    hover_timeout_ = 0;
  }
}

void PlaylistListView::StartDragHover(const std::string &name) {
  if (!PlaylistListLook::ShouldRestartDragHover(name, current_)) {
    CancelDragHover();
    return;
  }
  if (name == hover_name_ && hover_timeout_) {
    return;
  }
  CancelDragHover();
  hover_name_ = name;
  hover_timeout_ = g_timeout_add(PlaylistListLook::kDragHoverTimeoutMs, [](gpointer data) -> gboolean {
    auto *self = static_cast<PlaylistListView *>(data);
    self->hover_timeout_ = 0;
    const std::string hovered = self->hover_name_;
    self->hover_name_.clear();
    if (self->activate_ && !hovered.empty()) {
      self->activate_(hovered);
    }
    return G_SOURCE_REMOVE;
  }, this);
}

void PlaylistListView::SelectFolder(const std::string &path) {
  if (path.empty()) {
    return;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(list_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "playlist-folder")) != 1) {
      continue;
    }
    const char *row_path = static_cast<const char *>(g_object_get_data(G_OBJECT(child), "playlist-path"));
    if (!row_path || path != row_path) {
      continue;
    }
    gtk_list_box_unselect_all(GTK_LIST_BOX(list_));
    gtk_list_box_select_row(GTK_LIST_BOX(list_), GTK_LIST_BOX_ROW(child));
    gtk_widget_grab_focus(child);
    ListBoxEnsureVisible::Row(list_, child);
    return;
  }
}

void PlaylistListView::SelectName(const std::string &name) {
  if (name.empty()) {
    return;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(list_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    const char *row_name = static_cast<const char *>(g_object_get_data(G_OBJECT(child), "playlist-name"));
    if (!row_name || name != row_name) {
      continue;
    }
    gtk_list_box_unselect_all(GTK_LIST_BOX(list_));
    gtk_list_box_select_row(GTK_LIST_BOX(list_), GTK_LIST_BOX_ROW(child));
    gtk_widget_grab_focus(child);
    ListBoxEnsureVisible::Row(list_, child);
    return;
  }
}

void PlaylistListView::Refresh(const std::vector<PlaylistListDrop::Row> &rows, const std::string &current, const std::string &active,
                              PlaylistListLook::Playback playback) {
  CancelDragHover();
  favorites_.clear();
  current_ = current;
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (PlaylistListLook::ShouldShowEmptyHint(rows.size())) {
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
    GtkWidget *label = gtk_label_new(Translations::CStr(PlaylistListLook::EmptyHint()));
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_widget_add_css_class(label, "dim-label");
    gtk_widget_set_margin_start(label, 16);
    gtk_widget_set_margin_end(label, 16);
    gtk_widget_set_margin_top(label, 24);
    gtk_widget_set_margin_bottom(label, 24);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
    NotifySelectionChanged();
    return;
  }
  for (const PlaylistListDrop::Row &item : rows) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(outer, 8 + item.depth * 12);
    gtk_widget_set_margin_end(outer, 8);
    gtk_widget_set_margin_top(outer, 4);
    gtk_widget_set_margin_bottom(outer, 4);
    if (item.folder) {
      GtkWidget *toggle = gtk_button_new_from_icon_name(item.expanded ? "pan-down-symbolic" : "pan-end-symbolic");
      gtk_widget_add_css_class(toggle, "flat");
      gtk_widget_add_css_class(toggle, "circular");
      gtk_widget_set_tooltip_text(toggle, item.expanded ? Translations::CStr("Collapse") : Translations::CStr("Expand"));
      g_object_set_data_full(G_OBJECT(toggle), "playlist-path", g_strdup(item.path.c_str()), g_free);
      g_signal_connect(toggle, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                         auto *self = static_cast<PlaylistListView *>(data);
                         const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "playlist-path"));
                         if (path && self->toggle_) {
                           self->toggle_(path);
                         }
                       }),
                       this);
      gtk_box_append(GTK_BOX(outer), toggle);
      GtkWidget *icon = gtk_image_new_from_icon_name("folder-symbolic");
      gtk_box_append(GTK_BOX(outer), icon);
    } else if (const char *icon_name =
                   PlaylistListLook::PlaybackIconName(PlaylistListLook::IsActiveName(item.name, active), playback)) {
      GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
      gtk_box_append(GTK_BOX(outer), icon);
    }
    GtkWidget *label = gtk_label_new(item.name.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_box_append(GTK_BOX(outer), label);
    if (!item.folder) {
      auto favorite = std::make_unique<FavoriteWidget>(item.id, item.favorite);
      favorite->SetChangedCallback([this, name = item.name](int, bool on) {
        if (favorite_) {
          favorite_(name, on);
        }
      });
      gtk_box_append(GTK_BOX(outer), favorite->widget());
      favorites_.push_back(std::move(favorite));
    }
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), outer);
    g_object_set_data_full(G_OBJECT(row), "playlist-name", g_strdup(item.folder ? "" : item.name.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(row), "playlist-path", g_strdup(item.path.c_str()), g_free);
    g_object_set_data(G_OBJECT(row), "playlist-folder", GINT_TO_POINTER(item.folder ? 1 : 2));
    g_object_set_data(G_OBJECT(row), "playlist-expanded", GINT_TO_POINTER(item.expanded ? 1 : 2));
    g_object_set_data(G_OBJECT(row), "playlist-id", GINT_TO_POINTER(item.id + 1));
    if (!item.folder && item.name == current) {
      gtk_widget_add_css_class(row, "accent");
    }
    SetupRowDrop(row, item);
    if (!item.folder) {
      SetupRowDrag(row, item.name);
    }
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
  SelectName(current);
  NotifySelectionChanged();
}

void PlaylistListView::SetupRowDrop(GtkWidget *row, const PlaylistListDrop::Row &item) {
  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_drop_target_set_actions(target, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));
  gtk_drop_target_set_preload(target, TRUE);
  g_object_set_data_full(G_OBJECT(target), "playlist-name", g_strdup(item.folder ? item.path.c_str() : item.name.c_str()), g_free);
  g_object_set_data(G_OBJECT(target), "playlist-folder", GINT_TO_POINTER(item.folder ? 1 : 2));
  g_signal_connect(target, "motion", G_CALLBACK((+[](GtkDropTarget *t, gdouble, gdouble, gpointer data) -> GdkDragAction {
                     auto *self = static_cast<PlaylistListView *>(data);
                     const GValue *value = gtk_drop_target_get_value(t);
                     if (!value || !G_VALUE_HOLDS_STRING(value)) {
                       return static_cast<GdkDragAction>(0);
                     }
                     const char *text = g_value_get_string(value);
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(t), "playlist-name"));
                     const bool folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(t), "playlist-folder")) == 1;
                     if (!folder && name && PlaylistListLook::ShouldStartDragHover(text ? text : "")) {
                       self->StartDragHover(name);
                       return GDK_ACTION_COPY;
                     }
                     self->CancelDragHover();
                     return GDK_ACTION_COPY;
                   })),
                   this);
  g_signal_connect(target, "leave", G_CALLBACK(+[](GtkDropTarget *, gpointer data) {
                     static_cast<PlaylistListView *>(data)->CancelDragHover();
                   }),
                   this);
  g_signal_connect(target, "drop", G_CALLBACK((+[](GtkDropTarget *t, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistListView *>(data);
                     self->CancelDragHover();
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(t), "playlist-name"));
                     const bool folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(t), "playlist-folder")) == 1;
                     if (!self->drop_ || !name || !G_VALUE_HOLDS_STRING(value)) {
                       return FALSE;
                     }
                     const char *text = g_value_get_string(value);
                     if (!text || !*text) {
                       return FALSE;
                     }
                     self->drop_(name, text, folder);
                     return TRUE;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(target));
}

void PlaylistListView::SetupRowDrag(GtkWidget *row, const std::string &name) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_MOVE);
  g_object_set_data_full(G_OBJECT(src), "playlist-name", g_strdup(name.c_str()), g_free);
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer) -> GdkContentProvider * {
                     const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(s), "playlist-name"));
                     if (!name || !*name) {
                       return nullptr;
                     }
                     const std::string payload = PlaylistListDrop::MovePayload(name);
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

std::string PlaylistListView::SelectedName() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row) {
    return {};
  }
  const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-name"));
  return name ? name : "";
}

std::string PlaylistListView::SelectedFolderPath() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row || GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "playlist-folder")) != 1) {
    return {};
  }
  const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "playlist-path"));
  return path ? path : "";
}

bool PlaylistListView::SelectedIsFolder() const { return !SelectedFolderPath().empty(); }
