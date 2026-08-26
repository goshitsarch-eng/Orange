#include "fileview/fileviewlist.h"

#include "fileview/fileviewdrag.h"
#include "utilities/fileutils.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"
#include "widgets/listboxtreepressgtk.h"

#include <algorithm>

FileViewList::FileViewList() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_hexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  ListBoxTreePressGtk::Attach(list_, this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<FileViewList *>(data);
                     const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path"));
                     if (path && self->activate_) {
                       self->activate_(path);
                     }
                   }),
                   this);
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(gesture));
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<FileViewList *>(data);
                     GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y));
                     if (row && !gtk_list_box_row_is_selected(row)) {
                       gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                       gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                     }
                     if (self->menu_) {
                       self->menu_(self->SelectedPaths());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType mods, gpointer data) -> gboolean {
                     return static_cast<FileViewList *>(data)->OnKeyPressed(keyval, mods);
                   })),
                   this);
}

FileViewList::~FileViewList() { ResetTypeAhead(); }

void FileViewList::SetNavigateCallback(NavigateCallback callback) { navigate_ = std::move(callback); }

void FileViewList::SetActivateCallback(ActivateCallback callback) { activate_ = std::move(callback); }

void FileViewList::SetEnqueueCallback(EnqueueCallback callback) { enqueue_ = std::move(callback); }

void FileViewList::SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }

void FileViewList::HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state) {
  (void)x;
  if (CollectionTreeClick::FromPress(button, n_press, state) != CollectionTreeClick::Action::Enqueue || !enqueue_) {
    return;
  }
  GtkListBoxRow *row = ListBoxTreePressGtk::RowAtY(list_, y);
  if (row && CollectionTreeClick::SelectRowBeforeEnqueue(gtk_list_box_row_is_selected(row))) {
    ListBoxTreePressGtk::SelectRowIfNeeded(list_, row);
  }
  enqueue_(SelectedPaths());
}

void FileViewList::Reload(const std::vector<std::string> &paths) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  for (const std::string &path : paths) {
    GtkWidget *row = gtk_list_box_row_new();
    const bool dir = FileUtils::IsDirectory(path);
    GtkWidget *label = gtk_label_new(((dir ? "📁 " : "🎵 ") + FileUtils::BaseName(path)).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "file-path", g_strdup(path.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(row), "file-label", g_strdup(FileUtils::BaseName(path).c_str()), g_free);
    SetupRowDrag(row, path);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void FileViewList::SetupRowDrag(GtkWidget *row, const std::string &path) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  g_object_set_data_full(G_OBJECT(src), "file-path", g_strdup(path.c_str()), g_free);
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<FileViewList *>(data);
                     const char *dragged = static_cast<const char *>(g_object_get_data(G_OBJECT(s), "file-path"));
                     const std::string payload =
                         FileViewDrag::DragPayload(FileViewDrag::PathsForDrag(self->SelectedPaths(), dragged ? dragged : ""));
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

std::vector<std::string> FileViewList::SelectedPaths() const {
  std::vector<std::string> paths;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        const char *path = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "file-path"));
        if (path) {
          static_cast<std::vector<std::string> *>(data)->emplace_back(path);
        }
      },
      &paths);
  return paths;
}

void FileViewList::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

std::vector<std::string> FileViewList::Labels() const {
  std::vector<std::string> labels;
  if (!list_) {
    return labels;
  }
  for (GtkWidget *child = gtk_widget_get_first_child(list_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    const char *label = static_cast<const char *>(g_object_get_data(G_OBJECT(child), "file-label"));
    labels.emplace_back(label ? label : "");
  }
  return labels;
}

gboolean FileViewList::OnKeyPressed(guint keyval, GdkModifierType mods) {
  const bool alt = (mods & GDK_ALT_MASK) != 0;
  const FileViewKeyboard::Action action = FileViewKeyboard::FromKey(keyval, alt);
  if (action == FileViewKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == FileViewKeyboard::Action::MoveUp || action == FileViewKeyboard::Action::MoveDown ||
      action == FileViewKeyboard::Action::First || action == FileViewKeyboard::Action::Last) {
    const int count = ListBoxKeyboardGtk::Count(list_);
    const int current = ListBoxKeyboardGtk::SelectedIndex(list_);
    ListBoxKeyboard::Action move = ListBoxKeyboard::Action::None;
    if (action == FileViewKeyboard::Action::MoveUp) {
      move = ListBoxKeyboard::Action::MoveUp;
    } else if (action == FileViewKeyboard::Action::MoveDown) {
      move = ListBoxKeyboard::Action::MoveDown;
    } else if (action == FileViewKeyboard::Action::First) {
      move = ListBoxKeyboard::Action::Home;
    } else {
      move = ListBoxKeyboard::Action::End;
    }
    ListBoxKeyboardGtk::SelectIndex(list_, ListBoxKeyboard::NextIndex(current, count, move));
    return TRUE;
  }
  if (action == FileViewKeyboard::Action::UpDir || action == FileViewKeyboard::Action::HistoryBack ||
      action == FileViewKeyboard::Action::HistoryForward || action == FileViewKeyboard::Action::Home) {
    if (navigate_) {
      navigate_(action);
    }
    return TRUE;
  }
  if (keyval == ListBoxKeyboard::kEscape) {
    ResetTypeAhead();
    return TRUE;
  }
  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch && g_unichar_isprint(ch) && !alt) {
    gchar utf8[8] = {};
    const gint len = g_unichar_to_utf8(ch, utf8);
    typeahead_.append(utf8, static_cast<size_t>(len));
    if (typeahead_timeout_) {
      g_source_remove(typeahead_timeout_);
    }
    typeahead_timeout_ = g_timeout_add(1000, [](gpointer data) -> gboolean {
      auto *self = static_cast<FileViewList *>(data);
      self->typeahead_timeout_ = 0;
      self->typeahead_.clear();
      return G_SOURCE_REMOVE;
    }, this);
    const int index = ListBoxKeyboard::FirstPrefixIndex(Labels(), typeahead_);
    if (index >= 0) {
      ListBoxKeyboardGtk::SelectIndex(list_, index);
    }
    return TRUE;
  }
  return FALSE;
}
