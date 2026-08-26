#include "smartplaylists/smartplaylistsview.h"

#include "smartplaylists/smartplaylistdrag.h"
#include "translations/translations.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"

SmartPlaylistsView::SmartPlaylistsView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<SmartPlaylistsView *>(data);
                     auto *item = static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(row), "item"));
                     if (item) {
                       self->Emit(*item, SmartPlaylistsAction::Activate);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<SmartPlaylistsView *>(data);
                     const SmartPlaylistsItem *item = nullptr;
                     if (GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y))) {
                       if (!gtk_list_box_row_is_selected(row)) {
                         gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                         gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                       }
                       item = static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(row), "item"));
                     }
                     self->ShowMenu(GTK_WIDGET(self->list_), item);
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<SmartPlaylistsView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

SmartPlaylistsView::~SmartPlaylistsView() { ResetTypeAhead(); }

void SmartPlaylistsView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean SmartPlaylistsView::OnKeyPressed(guint keyval) {
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if (action == ListBoxKeyboard::Action::Activate) {
    ListBoxKeyboardGtk::ActivateSelected(list_);
    return TRUE;
  }
  if (action == ListBoxKeyboard::Action::Delete) {
    if (const SmartPlaylistsItem *item = SelectedItem()) {
      Emit(*item, SmartPlaylistsAction::Delete);
    }
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
      auto *self = static_cast<SmartPlaylistsView *>(data);
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

void SmartPlaylistsView::Emit(const SmartPlaylistsItem &item, SmartPlaylistsAction action) {
  if (action == SmartPlaylistsAction::Activate && activate_) {
    activate_(item);
    return;
  }
  if (action == SmartPlaylistsAction::Delete && delete_) {
    delete_(item);
    return;
  }
  if (action_) {
    action_(item, action);
  }
}

void SmartPlaylistsView::ShowMenu(GtkWidget *relative, const SmartPlaylistsItem *item) {
  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, relative);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);
  auto add_button = [&](const char *label, SmartPlaylistsAction action) {
    GtkWidget *button = gtk_button_new_with_label(label);
    gtk_widget_add_css_class(button, "flat");
    auto *copy = new SmartPlaylistsItem(item ? *item : SmartPlaylistsItem{});
    g_object_set_data_full(G_OBJECT(button), "item", copy, [](gpointer p) { delete static_cast<SmartPlaylistsItem *>(p); });
    g_object_set_data(G_OBJECT(button), "action", GINT_TO_POINTER(static_cast<int>(action) + 1));
    g_object_set_data(G_OBJECT(button), "popover", popover);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<SmartPlaylistsView *>(data);
                       auto *saved = static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(btn), "item"));
                       if (auto *menu = GTK_POPOVER(g_object_get_data(G_OBJECT(btn), "popover"))) {
                         gtk_popover_popdown(menu);
                       }
                       if (saved) {
                         self->Emit(*saved, static_cast<SmartPlaylistsAction>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "action")) - 1));
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(box), button);
  };
  if (item && item->kind != SmartPlaylistsItem::Kind::Wizard) {
    add_button(Translations::CStr("Append to current playlist"), SmartPlaylistsAction::Append);
    add_button(Translations::CStr("Replace current playlist"), SmartPlaylistsAction::Replace);
    add_button(Translations::CStr("Open in new playlist"), SmartPlaylistsAction::OpenInNew);
    add_button(Translations::CStr("Queue track"), SmartPlaylistsAction::Queue);
    add_button(Translations::CStr("Play next"), SmartPlaylistsAction::QueueNext);
  }
  add_button(Translations::CStr("New smart playlist…"), SmartPlaylistsAction::New);
  if (item && item->kind == SmartPlaylistsItem::Kind::Saved) {
    add_button(Translations::CStr("Edit smart playlist…"), SmartPlaylistsAction::Edit);
    add_button(Translations::CStr("Delete smart playlist"), SmartPlaylistsAction::Delete);
  }
  add_button(Translations::CStr("Restore defaults"), SmartPlaylistsAction::RestoreDefaults);
  gtk_popover_set_child(GTK_POPOVER(popover), box);
  gtk_popover_popup(GTK_POPOVER(popover));
}

void SmartPlaylistsView::Reload(SmartPlaylistsModel *model) {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (!model) {
    return;
  }
  for (const SmartPlaylistsItem &item : model->items()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *label = gtk_label_new(item.title.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_box_append(GTK_BOX(box), label);
    auto *copy = new SmartPlaylistsItem(item);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    g_object_set_data_full(G_OBJECT(row), "item", copy, [](gpointer p) { delete static_cast<SmartPlaylistsItem *>(p); });
    if (SmartPlaylistDrag::CanDrag(item.kind == SmartPlaylistsItem::Kind::Wizard)) {
      SetupRowDrag(row, item);
    }
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void SmartPlaylistsView::SetupRowDrag(GtkWidget *row, const SmartPlaylistsItem &item) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  auto *copy = new SmartPlaylistsItem(item);
  g_object_set_data_full(G_OBJECT(src), "item", copy, [](gpointer p) { delete static_cast<SmartPlaylistsItem *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<SmartPlaylistsView *>(data);
                     auto *dragged = static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(s), "item"));
                     if (!dragged || !self->songs_ || !SmartPlaylistDrag::CanDrag(dragged->kind == SmartPlaylistsItem::Kind::Wizard)) {
                       return nullptr;
                     }
                     const std::string payload = SmartPlaylistDrag::DragPayload(self->songs_(*dragged));
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

const SmartPlaylistsItem *SmartPlaylistsView::SelectedItem() const {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row) {
    return nullptr;
  }
  return static_cast<SmartPlaylistsItem *>(g_object_get_data(G_OBJECT(row), "item"));
}

void SmartPlaylistsView::Trigger(SmartPlaylistsAction action) {
  const SmartPlaylistsItem *item = SelectedItem();
  SmartPlaylistsItem fallback;
  fallback.kind = SmartPlaylistsItem::Kind::Wizard;
  fallback.title = "Custom wizard…";
  fallback.key = "wizard";
  Emit(item ? *item : fallback, action);
}
