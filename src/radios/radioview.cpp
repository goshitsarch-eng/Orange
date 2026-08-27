#include "radios/radioview.h"

#include "radios/radiodrag.h"
#include "radios/radiotree.h"
#include "radios/radiotreeleft.h"
#include "widgets/listboxkeyboard.h"
#include "widgets/listboxkeyboardgtk.h"
#include "widgets/listboxtreepressgtk.h"

RadioView::RadioView() {
  widget_ = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(widget_, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget_), list_);
  ListBoxTreePressGtk::Attach(list_, this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<RadioView *>(data);
                     const RadioTree::Kind kind =
                         static_cast<RadioTree::Kind>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-kind")) - 1);
                     auto *channel = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(row), "channel"));
                     if (channel && self->activate_ && RadioTree::ActivatePlays(kind)) {
                       self->activate_(*channel);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<RadioView *>(data);
                     if (GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y))) {
                       if (!gtk_list_box_row_is_selected(row)) {
                         gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                         gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                       }
                     }
                     if (self->menu_) {
                       self->menu_(self->SelectedChannels());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<RadioView *>(data)->OnKeyPressed(keyval);
                   })),
                   this);
}

RadioView::~RadioView() { ResetTypeAhead(); }

void RadioView::HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state) {
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
  if (action != CollectionTreeClick::Action::ToggleExpand || !row || ListBoxTreePressGtk::OnExpandControl(list_, x, y) || !model_) {
    return;
  }
  const RadioTree::Kind kind = static_cast<RadioTree::Kind>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-kind")) - 1);
  if (RadioTree::ActivateExpands(kind)) {
    const auto source = static_cast<Song::Source>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-source")));
    model_->Toggle(source);
    Reload(model_);
  }
}

void RadioView::Reload(RadioModel *model) {
  model_ = model;
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  if (!model) {
    return;
  }
  for (const RadioTree::Row &entry : model->Rows()) {
    GtkWidget *row = gtk_list_box_row_new();
    g_object_set_data(G_OBJECT(row), "row-kind", GINT_TO_POINTER(static_cast<int>(entry.kind) + 1));
    g_object_set_data(G_OBJECT(row), "row-source", GINT_TO_POINTER(static_cast<int>(entry.source)));
    if (entry.kind == RadioTree::Kind::Service) {
      GtkWidget *label = gtk_label_new(RadioTree::ServiceLabel(entry.source, entry.child_count, model->Expanded(entry.source)).c_str());
      gtk_widget_add_css_class(label, "heading");
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_set_margin_start(label, 12);
      gtk_widget_set_margin_end(label, 12);
      gtk_widget_set_margin_top(label, 8);
      gtk_widget_set_margin_bottom(label, 8);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
      auto *children = new std::vector<RadioChannel>(RadioTree::ChannelsForSource(model->visible(), entry.source));
      g_object_set_data_full(G_OBJECT(row), "channels", children, [](gpointer p) { delete static_cast<std::vector<RadioChannel> *>(p); });
    } else {
      GtkWidget *label = gtk_label_new(model->Label(entry.channel).c_str());
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_set_margin_start(label, 28);
      gtk_widget_set_margin_end(label, 12);
      gtk_widget_set_margin_top(label, 8);
      gtk_widget_set_margin_bottom(label, 8);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
      auto *copy = new RadioChannel(entry.channel);
      g_object_set_data_full(G_OBJECT(row), "channel", copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
      SetupRowDrag(row, entry.channel);
    }
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void RadioView::SetupRowDrag(GtkWidget *row, const RadioChannel &channel) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  auto *copy = new RadioChannel(channel);
  g_object_set_data_full(G_OBJECT(src), "channel", copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<RadioView *>(data);
                     auto *dragged = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(s), "channel"));
                     std::vector<RadioChannel> channels = dragged ? std::vector<RadioChannel>{*dragged} : std::vector<RadioChannel>{};
                     for (const RadioChannel &selected : self->SelectedChannels()) {
                       if (dragged && selected.url == dragged->url) {
                         channels = self->SelectedChannels();
                         break;
                       }
                     }
                     const std::string payload = RadioDrag::DragPayload(channels);
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

std::vector<RadioChannel> RadioView::SelectedChannels() const {
  std::vector<RadioChannel> channels;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        auto *out = static_cast<std::vector<RadioChannel> *>(data);
        if (auto *channel = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(row), "channel"))) {
          out->push_back(*channel);
          return;
        }
        if (auto *group = static_cast<std::vector<RadioChannel> *>(g_object_get_data(G_OBJECT(row), "channels"))) {
          out->insert(out->end(), group->begin(), group->end());
        }
      },
      &channels);
  return channels;
}

SongList RadioView::SelectedSongs() const { return RadioDrag::Songs(SelectedChannels()); }

void RadioView::SelectService(Song::Source source) {
  for (GtkWidget *child = gtk_widget_get_first_child(list_); child; child = gtk_widget_get_next_sibling(child)) {
    if (!GTK_IS_LIST_BOX_ROW(child)) {
      continue;
    }
    const RadioTree::Kind kind = static_cast<RadioTree::Kind>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-kind")) - 1);
    const auto row_source = static_cast<Song::Source>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "row-source")));
    if (kind != RadioTree::Kind::Service || row_source != source) {
      continue;
    }
    gtk_list_box_unselect_all(GTK_LIST_BOX(list_));
    gtk_list_box_select_row(GTK_LIST_BOX(list_), GTK_LIST_BOX_ROW(child));
    gtk_widget_grab_focus(child);
    return;
  }
}

bool RadioView::ApplyTreeLeft() {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
  if (!row || !model_) {
    return false;
  }
  const RadioTree::Kind kind = static_cast<RadioTree::Kind>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-kind")) - 1);
  const auto source = static_cast<Song::Source>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-source")));
  const bool expanded = model_->Expanded(source);
  if (!RadioTreeLeft::ShouldCollapse(kind, expanded)) {
    return false;
  }
  model_->Toggle(source);
  Reload(model_);
  SelectService(source);
  return true;
}

void RadioView::ResetTypeAhead() {
  typeahead_.clear();
  if (typeahead_timeout_) {
    g_source_remove(typeahead_timeout_);
    typeahead_timeout_ = 0;
  }
}

gboolean RadioView::OnKeyPressed(guint keyval) {
  if (keyval == GDK_KEY_F5 && refresh_) {
    refresh_();
    return TRUE;
  }
  const ListBoxKeyboard::Action action = ListBoxKeyboard::FromKey(keyval);
  if (keyval == ListBoxKeyboard::kLeft && ApplyTreeLeft()) {
    return TRUE;
  }
  if (keyval == ListBoxKeyboard::kRight) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list_));
    if (!row || !model_) {
      return FALSE;
    }
    const RadioTree::Kind kind = static_cast<RadioTree::Kind>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-kind")) - 1);
    const auto source = static_cast<Song::Source>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "row-source")));
    if (RadioTreeLeft::ShouldExpand(kind, model_->Expanded(source))) {
      model_->Toggle(source);
      Reload(model_);
      SelectService(source);
    }
    return kind == RadioTree::Kind::Service ? TRUE : FALSE;
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
      auto *self = static_cast<RadioView *>(data);
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
