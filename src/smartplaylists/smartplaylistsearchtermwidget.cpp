#include "smartplaylists/smartplaylistsearchtermwidget.h"

#include "dialogs/dialoghelpers.h"
#include "smartplaylists/smartplaylistsearchtermwidgetoverlay.h"
#include "smartplaylists/smartplaylistdateunits.h"
#include "smartplaylists/smartplaylisttagcompleter.h"
#include "settings/settingswheelthrough.h"
#include "smartplaylists/smartplaylisttermrow.h"
#include "widgets/ratingwidget.h"

#include <algorithm>
#include <cstdlib>

using DialogHelpers::DropDownFromNames;

SmartPlaylistSearchTermWidget::SmartPlaylistSearchTermWidget(SongList library) : library_(std::move(library)) {
  widget_ = gtk_overlay_new();
  row_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  field_ = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  op_ = DropDownFromNames(SmartPlaylistSearch::OpNames());
  remove_ = gtk_button_new_from_icon_name("list-remove-symbolic");
  gtk_widget_set_tooltip_text(remove_, "Remove search term");
  gtk_widget_set_valign(remove_, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(row_), field_);
  gtk_box_append(GTK_BOX(row_), op_);
  RebuildOps();
  RebuildValue();

  time_box_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_hexpand(time_box_, TRUE);
  time_hours_ = gtk_spin_button_new_with_range(0, 99, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(time_hours_), 0);
  time_minutes_ = gtk_spin_button_new_with_range(0, 59, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(time_minutes_), 0);
  time_seconds_ = gtk_spin_button_new_with_range(0, 59, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(time_seconds_), 0);
  SettingsWheelThrough::Attach(time_hours_);
  SettingsWheelThrough::Attach(time_minutes_);
  SettingsWheelThrough::Attach(time_seconds_);
  gtk_box_append(GTK_BOX(time_box_), time_hours_);
  gtk_box_append(GTK_BOX(time_box_), gtk_label_new("h"));
  gtk_box_append(GTK_BOX(time_box_), time_minutes_);
  gtk_box_append(GTK_BOX(time_box_), gtk_label_new("min"));
  gtk_box_append(GTK_BOX(time_box_), time_seconds_);
  gtk_box_append(GTK_BOX(time_box_), gtk_label_new("sec"));
  gtk_box_insert_child_after(GTK_BOX(row_), time_box_, value_ ? value_ : op_);
  gtk_widget_set_visible(time_box_, FALSE);

  rating_ = std::make_unique<RatingWidget>();
  rating_->set_rating(-1);
  gtk_widget_set_hexpand(rating_->widget(), TRUE);
  gtk_widget_set_valign(rating_->widget(), GTK_ALIGN_CENTER);
  gtk_box_insert_child_after(GTK_BOX(row_), rating_->widget(), time_box_);
  gtk_widget_set_visible(rating_->widget(), FALSE);
  rating_->SetChangedCallback([this](float) { EmitChanged(); });

  date_unit_ = DropDownFromNames(SmartPlaylistDateUnits::Names());
  gtk_drop_down_set_selected(GTK_DROP_DOWN(date_unit_), static_cast<guint>(SmartPlaylistDateType::Day));
  gtk_box_insert_child_after(GTK_BOX(row_), date_unit_, rating_->widget());
  gtk_widget_set_visible(date_unit_, FALSE);
  g_signal_connect(date_unit_, "notify::selected", G_CALLBACK((+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                   })),
                   this);

  range_box_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_hexpand(range_box_, TRUE);
  range_from_ = gtk_spin_button_new_with_range(0, 3650, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(range_from_), 0);
  range_to_ = gtk_spin_button_new_with_range(1, 3650, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(range_to_), 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_to_), 7);
  SettingsWheelThrough::Attach(range_from_);
  SettingsWheelThrough::Attach(range_to_);
  gtk_box_append(GTK_BOX(range_box_), range_from_);
  gtk_box_append(GTK_BOX(range_box_), gtk_label_new("and"));
  gtk_box_append(GTK_BOX(range_box_), range_to_);
  gtk_box_insert_child_after(GTK_BOX(row_), range_box_, date_unit_);
  gtk_widget_set_visible(range_box_, FALSE);
  g_signal_connect(range_from_, "value-changed", G_CALLBACK((+[](GtkSpinButton *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                   })),
                   this);
  g_signal_connect(range_to_, "value-changed", G_CALLBACK((+[](GtkSpinButton *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                   })),
                   this);

  g_signal_connect(time_hours_, "value-changed", G_CALLBACK((+[](GtkSpinButton *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                   })),
                   this);
  g_signal_connect(time_minutes_, "value-changed", G_CALLBACK((+[](GtkSpinButton *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                   })),
                   this);
  g_signal_connect(time_seconds_, "value-changed", G_CALLBACK((+[](GtkSpinButton *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                   })),
                   this);

  gtk_box_append(GTK_BOX(row_), remove_);
  gtk_overlay_set_child(GTK_OVERLAY(widget_), row_);
  overlay_ = gtk_button_new_with_label(SmartPlaylistTermRow::OverlayLabel());
  gtk_widget_add_css_class(overlay_, "suggested-action");
  gtk_widget_set_halign(overlay_, GTK_ALIGN_FILL);
  gtk_widget_set_valign(overlay_, GTK_ALIGN_FILL);
  gtk_widget_set_can_focus(overlay_, TRUE);
  gtk_overlay_add_overlay(GTK_OVERLAY(widget_), overlay_);
  SmartPlaylistSearchTermWidgetOverlay::Apply(widget_);
  g_signal_connect(overlay_, "map", G_CALLBACK((+[](GtkWidget *, gpointer data) {
                     static_cast<SmartPlaylistSearchTermWidget *>(data)->OnOverlayMapped();
                   })),
                   this);
  GtkEventController *overlay_keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(overlay_keys, GTK_PHASE_CAPTURE);
  g_signal_connect(overlay_keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
                     return static_cast<SmartPlaylistSearchTermWidget *>(data)->OnOverlayActivateKey(keyval) ? TRUE : FALSE;
                   })),
                   this);
  gtk_widget_add_controller(overlay_, overlay_keys);
  g_signal_connect(field_, "notify::selected", G_CALLBACK((+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     auto *self = static_cast<SmartPlaylistSearchTermWidget *>(data);
                     if (self->updating_) {
                       return;
                     }
                     self->updating_ = true;
                     self->RebuildOps();
                     self->RebuildValue();
                     self->updating_ = false;
                     self->EmitChanged();
                   })),
                   this);
  g_signal_connect(op_, "notify::selected", G_CALLBACK((+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     auto *self = static_cast<SmartPlaylistSearchTermWidget *>(data);
                     if (self->updating_) {
                       return;
                     }
                     self->updating_ = true;
                     self->RebuildValue();
                     self->updating_ = false;
                     self->EmitChanged();
                   })),
                   this);
  g_signal_connect(remove_, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<SmartPlaylistSearchTermWidget *>(data);
                     if (self->active_ && self->removed_) {
                       self->removed_();
                     }
                   })),
                   this);
  g_signal_connect(overlay_, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<SmartPlaylistSearchTermWidget *>(data);
                     if (!self->active_ && self->clicked_) {
                       self->clicked_();
                     }
                   })),
                   this);
  ApplyActive();
}

SmartPlaylistSearchTermWidget::~SmartPlaylistSearchTermWidget() = default;

void SmartPlaylistSearchTermWidget::OnOverlayMapped() {
  if (overlay_ && SmartPlaylistSearchTermWidgetOverlay::ShouldGrabOnShow(active_)) {
    gtk_widget_grab_focus(overlay_);
  }
}

bool SmartPlaylistSearchTermWidget::OnOverlayActivateKey(unsigned keyval) {
  if (!SmartPlaylistSearchTermWidgetOverlay::IsActivateKey(keyval) || active_) {
    return false;
  }
  if (clicked_) {
    clicked_();
  }
  return true;
}

void SmartPlaylistSearchTermWidget::SetActive(bool active) {
  active_ = active;
  ApplyActive();
}

void SmartPlaylistSearchTermWidget::ApplyActive() {
  gtk_widget_set_sensitive(row_, SmartPlaylistTermRow::RowSensitive(active_) ? TRUE : FALSE);
  if (remove_) {
    gtk_widget_set_visible(remove_, SmartPlaylistTermRow::ShowsRemove(active_) ? TRUE : FALSE);
  }
  if (overlay_) {
    gtk_widget_set_visible(overlay_, active_ ? FALSE : TRUE);
  }
}

void SmartPlaylistSearchTermWidget::RebuildOps() {
  const SmartPlaylistField field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  current_ops_ = SmartPlaylistSearch::OperatorsFor(field);
  GtkStringList *model = gtk_string_list_new(nullptr);
  for (SmartPlaylistOp op : current_ops_) {
    gtk_string_list_append(model, SmartPlaylistSearch::OpName(op).c_str());
  }
  const bool was_updating = updating_;
  updating_ = true;
  gtk_drop_down_set_model(GTK_DROP_DOWN(op_), G_LIST_MODEL(model));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(op_), 0);
  updating_ = was_updating;
}

void SmartPlaylistSearchTermWidget::RebuildValue() {
  const std::string previous = CurrentValue();
  if (value_) {
    gtk_box_remove(GTK_BOX(row_), value_);
    value_ = nullptr;
  }
  const SmartPlaylistField field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  const SmartPlaylistOp op = current_ops_.empty() ? SmartPlaylistOp::Contains
                                                  : current_ops_[std::min(current_ops_.size() - 1, static_cast<size_t>(gtk_drop_down_get_selected(GTK_DROP_DOWN(op_))))];
  editor_ = SmartPlaylistTermValue::EditorFor(SmartPlaylistSearch::KindOf(field), op);

  if (time_box_) {
    gtk_widget_set_visible(time_box_, editor_ == SmartPlaylistTermValue::Editor::Time ? TRUE : FALSE);
  }
  if (rating_) {
    gtk_widget_set_visible(rating_->widget(), editor_ == SmartPlaylistTermValue::Editor::Rating ? TRUE : FALSE);
  }
  if (date_unit_) {
    const bool show_unit = editor_ == SmartPlaylistTermValue::Editor::RelativeDays || editor_ == SmartPlaylistTermValue::Editor::RelativeRange;
    gtk_widget_set_visible(date_unit_, show_unit ? TRUE : FALSE);
  }
  if (range_box_) {
    gtk_widget_set_visible(range_box_, editor_ == SmartPlaylistTermValue::Editor::RelativeRange ? TRUE : FALSE);
  }

  switch (editor_) {
    case SmartPlaylistTermValue::Editor::Empty:
      value_ = gtk_label_new("");
      break;
    case SmartPlaylistTermValue::Editor::RelativeDays:
      value_ = gtk_spin_button_new_with_range(0, 3650, 1);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(value_), 0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(value_), 30);
      break;
    case SmartPlaylistTermValue::Editor::RelativeRange:
      if (range_from_) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_from_), 1);
      }
      if (range_to_) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_to_), 7);
      }
      break;
    case SmartPlaylistTermValue::Editor::Calendar:
      value_ = gtk_calendar_new();
      break;
    case SmartPlaylistTermValue::Editor::Number:
      value_ = gtk_spin_button_new_with_range(0, 1000000, 1);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(value_), 0);
      break;
    case SmartPlaylistTermValue::Editor::Text:
      value_ = gtk_entry_new();
      gtk_entry_set_placeholder_text(GTK_ENTRY(value_), "Value");
      break;
    case SmartPlaylistTermValue::Editor::Rating:
    case SmartPlaylistTermValue::Editor::Time:
      break;
  }

  if (value_) {
    gtk_widget_set_hexpand(value_, TRUE);
    SettingsWheelThrough::AttachSpin(value_);
    gtk_box_insert_child_after(GTK_BOX(row_), value_, op_);
  }
  if (range_box_) {
    gtk_box_reorder_child_after(GTK_BOX(row_), range_box_, value_ ? value_ : op_);
  }
  if (date_unit_) {
    GtkWidget *after = range_box_ && editor_ == SmartPlaylistTermValue::Editor::RelativeRange ? range_box_ : (value_ ? value_ : op_);
    gtk_box_reorder_child_after(GTK_BOX(row_), date_unit_, after);
  }
  if (!previous.empty()) {
    SetCurrentValue(previous);
  }
  AttachCompletion();
  ConnectValueSignals();
}

void SmartPlaylistSearchTermWidget::AttachCompletion() {
  if (!GTK_IS_ENTRY(value_)) {
    return;
  }
  const SmartPlaylistField field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  if (!SmartPlaylistTagCompleter::ShouldAttach(editor_, field)) {
    gtk_entry_set_completion(GTK_ENTRY(value_), nullptr);
    return;
  }
  GtkEntryCompletion *completion = gtk_entry_completion_new();
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
  for (const std::string &value : SmartPlaylistTagCompleter::ValuesFor(library_, field)) {
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, value.c_str(), -1);
  }
  gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
  gtk_entry_completion_set_text_column(completion, 0);
  gtk_entry_set_completion(GTK_ENTRY(value_), completion);
  g_object_unref(store);
  g_object_unref(completion);
}

void SmartPlaylistSearchTermWidget::ConnectValueSignals() {
  if (!value_ || GTK_IS_LABEL(value_)) {
    return;
  }
  if (GTK_IS_CALENDAR(value_)) {
    g_signal_connect(value_, "day-selected", G_CALLBACK((+[](GtkCalendar *, gpointer data) {
                       static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                     })),
                     this);
    return;
  }
  if (GTK_IS_SPIN_BUTTON(value_)) {
    g_signal_connect(value_, "value-changed", G_CALLBACK((+[](GtkSpinButton *, gpointer data) {
                       static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                     })),
                     this);
    return;
  }
  if (GTK_IS_EDITABLE(value_)) {
    g_signal_connect(value_, "changed", G_CALLBACK((+[](GtkEditable *, gpointer data) {
                       static_cast<SmartPlaylistSearchTermWidget *>(data)->EmitChanged();
                     })),
                     this);
  }
}

void SmartPlaylistSearchTermWidget::EmitChanged() {
  if (!updating_ && active_ && changed_) {
    changed_();
  }
}

std::string SmartPlaylistSearchTermWidget::CurrentValue() const {
  if (editor_ == SmartPlaylistTermValue::Editor::Empty) {
    return {};
  }
  if (editor_ == SmartPlaylistTermValue::Editor::Rating && rating_) {
    return SmartPlaylistTermValue::FormatRating(rating_->rating());
  }
  if (editor_ == SmartPlaylistTermValue::Editor::RelativeRange && range_from_) {
    return std::to_string(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(range_from_)));
  }
  if (editor_ == SmartPlaylistTermValue::Editor::Time && time_hours_ && time_minutes_ && time_seconds_) {
    return std::to_string(SmartPlaylistTermValue::TimeToSeconds(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(time_hours_)),
                                                               gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(time_minutes_)),
                                                               gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(time_seconds_))));
  }
  if (!value_) {
    return {};
  }
  if (GTK_IS_CALENDAR(value_)) {
    GDateTime *date = gtk_calendar_get_date(GTK_CALENDAR(value_));
    if (!date) {
      return {};
    }
    const std::string formatted = SmartPlaylistTermValue::FormatDate(g_date_time_get_year(date), g_date_time_get_month(date),
                                                                     g_date_time_get_day_of_month(date));
    g_date_time_unref(date);
    return formatted;
  }
  if (GTK_IS_SPIN_BUTTON(value_)) {
    return std::to_string(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(value_)));
  }
  if (GTK_IS_EDITABLE(value_)) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(value_));
    return text ? text : "";
  }
  return {};
}

void SmartPlaylistSearchTermWidget::SetCurrentValue(const std::string &value) {
  if (value.empty()) {
    return;
  }
  if (editor_ == SmartPlaylistTermValue::Editor::Rating && rating_) {
    rating_->set_rating(static_cast<float>(g_ascii_strtod(value.c_str(), nullptr)));
    return;
  }
  if (editor_ == SmartPlaylistTermValue::Editor::RelativeRange && range_from_) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_from_), g_ascii_strtod(value.c_str(), nullptr));
    return;
  }
  if (editor_ == SmartPlaylistTermValue::Editor::Time && time_hours_ && time_minutes_ && time_seconds_) {
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    SmartPlaylistTermValue::SecondsToTime(static_cast<int>(g_ascii_strtod(value.c_str(), nullptr)), &hours, &minutes, &seconds);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(time_hours_), hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(time_minutes_), minutes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(time_seconds_), seconds);
    return;
  }
  if (!value_) {
    return;
  }
  if (GTK_IS_CALENDAR(value_)) {
    int year = 0;
    int month = 0;
    int day = 0;
    GDateTime *date = nullptr;
    if (SmartPlaylistTermValue::ParseDate(value, &year, &month, &day)) {
      date = g_date_time_new_local(year, month, day, 0, 0, 0);
    }
    else {
      const gint64 unix_time = std::strtoll(value.c_str(), nullptr, 10);
      if (unix_time > 0) {
        date = g_date_time_new_from_unix_local(unix_time);
      }
    }
    if (date) {
      gtk_calendar_select_day(GTK_CALENDAR(value_), date);
      g_date_time_unref(date);
    }
    return;
  }
  if (GTK_IS_SPIN_BUTTON(value_)) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(value_), g_ascii_strtod(value.c_str(), nullptr));
    return;
  }
  if (GTK_IS_EDITABLE(value_)) {
    gtk_editable_set_text(GTK_EDITABLE(value_), value.c_str());
  }
}

SmartPlaylistTerm SmartPlaylistSearchTermWidget::Term() const {
  SmartPlaylistTerm term;
  term.field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  const guint index = gtk_drop_down_get_selected(GTK_DROP_DOWN(op_));
  term.op = index < current_ops_.size() ? current_ops_[index] : SmartPlaylistOp::Contains;
  term.value = CurrentValue();
  if (date_unit_) {
    term.date_type = SmartPlaylistDateUnits::FromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(date_unit_))));
  }
  if (editor_ == SmartPlaylistTermValue::Editor::RelativeRange && range_to_) {
    term.second_value = std::to_string(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(range_to_)));
  }
  return term;
}

void SmartPlaylistSearchTermWidget::SetTerm(const SmartPlaylistTerm &term) {
  updating_ = true;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(field_), static_cast<guint>(term.field));
  RebuildOps();
  guint op_index = 0;
  for (size_t i = 0; i < current_ops_.size(); ++i) {
    if (current_ops_[i] == term.op) {
      op_index = static_cast<guint>(i);
      break;
    }
  }
  gtk_drop_down_set_selected(GTK_DROP_DOWN(op_), op_index);
  RebuildValue();
  SetCurrentValue(term.value);
  if (date_unit_) {
    gtk_drop_down_set_selected(GTK_DROP_DOWN(date_unit_), static_cast<guint>(SmartPlaylistDateUnits::ToIndex(term.date_type)));
  }
  if (range_to_ && !term.second_value.empty()) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_to_), g_ascii_strtod(term.second_value.c_str(), nullptr));
  }
  updating_ = false;
}

bool SmartPlaylistSearchTermWidget::IsEmpty() const {
  const SmartPlaylistTerm term = Term();
  return term.value.empty() && term.op != SmartPlaylistOp::Empty && term.op != SmartPlaylistOp::NotEmpty;
}
