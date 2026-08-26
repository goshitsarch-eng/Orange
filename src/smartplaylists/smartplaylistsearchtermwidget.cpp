#include "smartplaylists/smartplaylistsearchtermwidget.h"

#include "dialogs/dialoghelpers.h"

#include <algorithm>

using DialogHelpers::DropDownFromNames;

SmartPlaylistSearchTermWidget::SmartPlaylistSearchTermWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  field_ = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  op_ = DropDownFromNames(SmartPlaylistSearch::OpNames());
  gtk_box_append(GTK_BOX(widget_), field_);
  gtk_box_append(GTK_BOX(widget_), op_);
  RebuildOps();
  RebuildValue();
  g_signal_connect(field_, "notify::selected", G_CALLBACK(+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     auto *self = static_cast<SmartPlaylistSearchTermWidget *>(data);
                     if (!self->updating_) {
                       self->RebuildOps();
                       self->RebuildValue();
                     }
                   }),
                   this);
  g_signal_connect(op_, "notify::selected", G_CALLBACK(+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     auto *self = static_cast<SmartPlaylistSearchTermWidget *>(data);
                     if (!self->updating_) {
                       self->RebuildValue();
                     }
                   }),
                   this);
}

void SmartPlaylistSearchTermWidget::RebuildOps() {
  const SmartPlaylistField field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  current_ops_ = SmartPlaylistSearch::OperatorsFor(field);
  GtkStringList *model = gtk_string_list_new(nullptr);
  for (SmartPlaylistOp op : current_ops_) {
    gtk_string_list_append(model, SmartPlaylistSearch::OpName(op).c_str());
  }
  updating_ = true;
  gtk_drop_down_set_model(GTK_DROP_DOWN(op_), G_LIST_MODEL(model));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(op_), 0);
  updating_ = false;
}

void SmartPlaylistSearchTermWidget::RebuildValue() {
  const std::string previous = CurrentValue();
  if (value_) {
    gtk_box_remove(GTK_BOX(widget_), value_);
    value_ = nullptr;
  }
  const SmartPlaylistField field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  const SmartPlaylistOp op = current_ops_.empty() ? SmartPlaylistOp::Contains
                                                  : current_ops_[std::min(current_ops_.size() - 1, static_cast<size_t>(gtk_drop_down_get_selected(GTK_DROP_DOWN(op_))))];
  const SmartPlaylistFieldKind kind = SmartPlaylistSearch::KindOf(field);
  if (op == SmartPlaylistOp::Empty || op == SmartPlaylistOp::NotEmpty) {
    value_ = gtk_label_new("");
  } else if (op == SmartPlaylistOp::RelativeDate) {
    value_ = gtk_spin_button_new_with_range(1, 3650, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(value_), 30);
  } else if (kind == SmartPlaylistFieldKind::Date || op == SmartPlaylistOp::NumericDate) {
    value_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(value_), "YYYY-MM-DD");
  } else if (kind == SmartPlaylistFieldKind::Number || kind == SmartPlaylistFieldKind::Rating || kind == SmartPlaylistFieldKind::Time) {
    const double max = kind == SmartPlaylistFieldKind::Rating ? 1.0 : 1000000.0;
    const double step = kind == SmartPlaylistFieldKind::Rating ? 0.1 : 1.0;
    value_ = gtk_spin_button_new_with_range(0, max, step);
  } else {
    value_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(value_), "Value");
  }
  gtk_widget_set_hexpand(value_, TRUE);
  gtk_box_append(GTK_BOX(widget_), value_);
  if (!previous.empty()) {
    SetCurrentValue(previous);
  }
}

std::string SmartPlaylistSearchTermWidget::CurrentValue() const {
  if (!value_) {
    return {};
  }
  if (GTK_IS_SPIN_BUTTON(value_)) {
    if (gtk_spin_button_get_digits(GTK_SPIN_BUTTON(value_)) > 0) {
      return std::to_string(gtk_spin_button_get_value(GTK_SPIN_BUTTON(value_)));
    }
    return std::to_string(static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(value_))));
  }
  if (GTK_IS_EDITABLE(value_)) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(value_));
    return text ? text : "";
  }
  return {};
}

void SmartPlaylistSearchTermWidget::SetCurrentValue(const std::string &value) {
  if (!value_ || value.empty()) {
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
  updating_ = false;
  RebuildValue();
  SetCurrentValue(term.value);
}

bool SmartPlaylistSearchTermWidget::IsEmpty() const {
  const SmartPlaylistTerm term = Term();
  return term.value.empty() && term.op != SmartPlaylistOp::Empty && term.op != SmartPlaylistOp::NotEmpty;
}
