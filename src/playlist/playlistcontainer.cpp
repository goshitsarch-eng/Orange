#include "playlist/playlistcontainer.h"

#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "filterparser/filterparser.h"
#include "playlist/playlistfiltersync.h"
#include "playlist/playlistundostate.h"
#include "translations/translations.h"
#include "widgets/filtersearchkeyboard.h"

PlaylistContainer::PlaylistContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)),
      tab_bar_(std::make_unique<PlaylistTabBar>()),
      view_(std::make_unique<PlaylistView>()),
      dynamic_controls_(std::make_unique<DynamicPlaylistControls>()) {
  toolbar_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(toolbar_, 8);
  gtk_widget_set_margin_end(toolbar_, 8);
  gtk_widget_set_margin_top(toolbar_, 8);
  gtk_widget_set_margin_bottom(toolbar_, 4);
  auto add_tool = [&](const char *icon, const char *tooltip, const char *name) -> GtkWidget * {
    GtkWidget *button = gtk_button_new_from_icon_name(icon);
    gtk_widget_set_tooltip_text(button, tooltip);
    g_object_set_data_full(G_OBJECT(button), "action", g_strdup(name), g_free);
    gtk_box_append(GTK_BOX(toolbar_), button);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<PlaylistContainer *>(data);
                       const char *action = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "action"));
                       auto *cb = static_cast<ActionCallback *>(g_object_get_data(G_OBJECT(self->widget_), action ? action : ""));
                       if (cb && *cb) {
                         (*cb)();
                       }
                     }),
                     this);
    return button;
  };
  add_tool("document-new-symbolic", Translations::Tr("New playlist").c_str(), "new");
  add_tool("document-open-symbolic", Translations::Tr("Load playlist").c_str(), "load");
  add_tool("document-save-symbolic", Translations::Tr("Save playlist").c_str(), "save");
  clear_button_ = add_tool("edit-clear-all-symbolic", Translations::Tr("Clear playlist").c_str(), "clear");
  undo_button_ = add_tool("edit-undo-symbolic", Translations::CStr(PlaylistUndoState::UndoTooltip(false)), "undo");
  redo_button_ = add_tool("edit-redo-symbolic", Translations::CStr(PlaylistUndoState::RedoTooltip(false)), "redo");
  UpdateUndoRedoChrome(false, false);

  auto make_menu_button = [](const char *icon, const char *tooltip) {
    GtkWidget *button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(button), icon);
    gtk_menu_button_set_always_show_arrow(GTK_MENU_BUTTON(button), FALSE);
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_tooltip_text(button, tooltip);
    return button;
  };
  repeat_button_ = make_menu_button("media-playlist-repeat-symbolic", Translations::CStr(PlaylistSequence::RepeatButtonTooltip()));
  shuffle_button_ = make_menu_button("media-playlist-shuffle-symbolic", Translations::CStr(PlaylistSequence::ShuffleButtonTooltip()));

  GtkWidget *repeat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *repeat_group = nullptr;
  for (PlaylistSequence::RepeatMode mode : PlaylistSequence::RepeatModes()) {
    GtkWidget *item = gtk_check_button_new_with_label(PlaylistSequence::RepeatLabel(mode));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(item), repeat_group ? GTK_CHECK_BUTTON(repeat_group) : nullptr);
    if (!repeat_group) {
      repeat_group = item;
      gtk_check_button_set_active(GTK_CHECK_BUTTON(item), TRUE);
    }
    g_object_set_data(G_OBJECT(item), "mode", GINT_TO_POINTER(static_cast<int>(mode) + 1));
    g_signal_connect(item, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                       if (!gtk_check_button_get_active(button)) {
                         return;
                       }
                       auto *self = static_cast<PlaylistContainer *>(data);
                       if (self->updating_sequence_) {
                         return;
                       }
                       const int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "mode")) - 1;
                       self->SetRepeatMode(static_cast<PlaylistSequence::RepeatMode>(mode));
                       if (self->repeat_changed_) {
                         self->repeat_changed_(static_cast<PlaylistSequence::RepeatMode>(mode));
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(repeat_box), item);
    repeat_items_.push_back(item);
  }
  GtkWidget *repeat_popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER(repeat_popover), repeat_box);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(repeat_button_), repeat_popover);

  GtkWidget *shuffle_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *shuffle_group = nullptr;
  for (PlaylistSequence::ShuffleMode mode : PlaylistSequence::ShuffleModes()) {
    GtkWidget *item = gtk_check_button_new_with_label(PlaylistSequence::ShuffleLabel(mode));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(item), shuffle_group ? GTK_CHECK_BUTTON(shuffle_group) : nullptr);
    if (!shuffle_group) {
      shuffle_group = item;
      gtk_check_button_set_active(GTK_CHECK_BUTTON(item), TRUE);
    }
    g_object_set_data(G_OBJECT(item), "mode", GINT_TO_POINTER(static_cast<int>(mode) + 1));
    g_signal_connect(item, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                       if (!gtk_check_button_get_active(button)) {
                         return;
                       }
                       auto *self = static_cast<PlaylistContainer *>(data);
                       if (self->updating_sequence_) {
                         return;
                       }
                       const int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "mode")) - 1;
                       self->SetShuffleMode(static_cast<PlaylistSequence::ShuffleMode>(mode));
                       if (self->shuffle_changed_) {
                         self->shuffle_changed_(static_cast<PlaylistSequence::ShuffleMode>(mode));
                       }
                     }),
                     this);
    gtk_box_append(GTK_BOX(shuffle_box), item);
    shuffle_items_.push_back(item);
  }
  GtkWidget *shuffle_popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER(shuffle_popover), shuffle_box);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(shuffle_button_), shuffle_popover);

  gtk_box_append(GTK_BOX(toolbar_), repeat_button_);
  gtk_box_append(GTK_BOX(toolbar_), shuffle_button_);
  filter_entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(filter_entry_), Translations::Tr("Filter playlist").c_str());
  gtk_widget_set_tooltip_text(filter_entry_, Translations::CStr(FilterParser::ToolTip().c_str()));
  gtk_widget_set_hexpand(filter_entry_, TRUE);
  g_signal_connect(filter_entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *entry, gpointer data) {
                     auto *self = static_cast<PlaylistContainer *>(data);
                     if (self->updating_filter_) {
                       return;
                     }
                     self->filter_ = gtk_editable_get_text(GTK_EDITABLE(entry));
                     if (self->filter_changed_) {
                       self->filter_changed_(self->filter_);
                     }
                   }),
                   this);
  g_signal_connect(filter_entry_, "activate", G_CALLBACK(+[](GtkSearchEntry *, gpointer data) {
                     static_cast<PlaylistContainer *>(data)->view()->FilterReturnPressed();
                   }),
                   this);
  GtkEventController *filter_keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(filter_entry_, filter_keys);
  g_signal_connect(filter_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     auto *self = static_cast<PlaylistContainer *>(data);
                     const FilterSearchKeyboard::Action action = FilterSearchKeyboard::FromSearchKey(keyval);
                     if (action == FilterSearchKeyboard::Action::MoveUp || action == FilterSearchKeyboard::Action::MoveDown) {
                       self->view()->FocusAndMove(keyval);
                       return TRUE;
                     }
                     if (action == FilterSearchKeyboard::Action::Clear) {
                       gtk_editable_set_text(GTK_EDITABLE(self->filter_entry_), "");
                       return TRUE;
                     }
                     return FALSE;
                   })),
                   this);
  gtk_box_append(GTK_BOX(toolbar_), filter_entry_);
  view_->SetFocusFilterCallback([this]() { FocusFilter(); });
  summary_ = gtk_label_new("");
  gtk_widget_add_css_class(summary_, "dim-label");
  gtk_widget_set_visible(summary_, FALSE);
  gtk_box_append(GTK_BOX(toolbar_), summary_);
  gtk_widget_set_margin_start(tab_bar_->widget(), 8);
  gtk_widget_set_margin_end(tab_bar_->widget(), 8);
  gtk_box_append(GTK_BOX(widget_), toolbar_);
  ApplyLook();
  gtk_box_append(GTK_BOX(widget_), tab_bar_->widget());
  gtk_box_append(GTK_BOX(widget_), dynamic_controls_->widget());
  gtk_box_append(GTK_BOX(widget_), view_->widget());
}

void PlaylistContainer::SetFilterChangedCallback(const std::function<void(const std::string &)> &callback) {
  filter_changed_ = callback;
}

void PlaylistContainer::FocusFilter() {
  if (filter_entry_) {
    gtk_widget_grab_focus(filter_entry_);
  }
}

void PlaylistContainer::SetFilterText(const std::string &text) {
  if (!filter_entry_ || !PlaylistFilterSync::ShouldSyncEntry(filter_, text)) {
    return;
  }
  updating_filter_ = true;
  filter_ = text;
  gtk_editable_set_text(GTK_EDITABLE(filter_entry_), text.c_str());
  updating_filter_ = false;
}

void PlaylistContainer::UpdateNoMatchesOverlay() {
  if (view_) {
    view_->UpdateNoMatchesOverlay();
  }
}

void PlaylistContainer::UpdateUndoRedoChrome(bool can_undo, bool can_redo) {
  if (undo_button_) {
    gtk_widget_set_sensitive(undo_button_, PlaylistUndoState::UndoEnabled(can_undo) ? TRUE : FALSE);
    gtk_widget_set_tooltip_text(undo_button_, Translations::CStr(PlaylistUndoState::UndoTooltip(can_undo)));
  }
  if (redo_button_) {
    gtk_widget_set_sensitive(redo_button_, PlaylistUndoState::RedoEnabled(can_redo) ? TRUE : FALSE);
    gtk_widget_set_tooltip_text(redo_button_, Translations::CStr(PlaylistUndoState::RedoTooltip(can_redo)));
  }
}

void PlaylistContainer::SetActionCallback(const char *name, ActionCallback callback) {
  auto *cb = new ActionCallback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget_), name, cb, [](gpointer p) { delete static_cast<ActionCallback *>(p); });
}

void PlaylistContainer::SetRepeatChangedCallback(const std::function<void(PlaylistSequence::RepeatMode)> &callback) {
  repeat_changed_ = callback;
}

void PlaylistContainer::SetShuffleChangedCallback(const std::function<void(PlaylistSequence::ShuffleMode)> &callback) {
  shuffle_changed_ = callback;
}

void PlaylistContainer::SetRepeatMode(PlaylistSequence::RepeatMode mode) {
  updating_sequence_ = true;
  for (GtkWidget *item : repeat_items_) {
    const int item_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "mode")) - 1;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(item), item_mode == static_cast<int>(mode));
  }
  if (repeat_button_) {
    gtk_widget_set_tooltip_text(repeat_button_, PlaylistSequence::RepeatLabel(mode));
    if (PlaylistSequence::RepeatActive(mode)) {
      gtk_widget_add_css_class(repeat_button_, "accent");
    } else {
      gtk_widget_remove_css_class(repeat_button_, "accent");
    }
  }
  updating_sequence_ = false;
}

void PlaylistContainer::SetShuffleMode(PlaylistSequence::ShuffleMode mode) {
  updating_sequence_ = true;
  for (GtkWidget *item : shuffle_items_) {
    const int item_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "mode")) - 1;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(item), item_mode == static_cast<int>(mode));
  }
  if (shuffle_button_) {
    gtk_widget_set_tooltip_text(shuffle_button_, PlaylistSequence::ShuffleLabel(mode));
    if (PlaylistSequence::ShuffleActive(mode)) {
      gtk_widget_add_css_class(shuffle_button_, "accent");
    } else {
      gtk_widget_remove_css_class(shuffle_button_, "accent");
    }
  }
  updating_sequence_ = false;
}

void PlaylistContainer::SetSummary(const std::string &text) { gtk_label_set_text(GTK_LABEL(summary_), text.c_str()); }

void PlaylistContainer::ApplyLook() {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  if (toolbar_) {
    gtk_widget_set_visible(toolbar_, settings.BoolValue(PlaylistSettings::kShowToolbar, PlaylistSettings::kDefaultShowToolbar));
  }
  if (clear_button_) {
    gtk_widget_set_visible(clear_button_, settings.BoolValue(PlaylistSettings::kPlaylistClear, PlaylistSettings::kDefaultPlaylistClear));
  }
}
