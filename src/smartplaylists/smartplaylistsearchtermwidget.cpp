#include "smartplaylists/smartplaylistsearchtermwidget.h"

#include "dialogs/dialoghelpers.h"

using DialogHelpers::DropDownFromNames;

SmartPlaylistSearchTermWidget::SmartPlaylistSearchTermWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  field_ = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  op_ = DropDownFromNames(SmartPlaylistSearch::OpNames());
  value_ = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(value_), "Value");
  gtk_widget_set_hexpand(value_, TRUE);
  gtk_box_append(GTK_BOX(widget_), field_);
  gtk_box_append(GTK_BOX(widget_), op_);
  gtk_box_append(GTK_BOX(widget_), value_);
}

SmartPlaylistTerm SmartPlaylistSearchTermWidget::Term() const {
  SmartPlaylistTerm term;
  term.field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(field_))));
  term.op = SmartPlaylistSearch::OpFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(op_))));
  term.value = gtk_editable_get_text(GTK_EDITABLE(value_));
  return term;
}

void SmartPlaylistSearchTermWidget::SetTerm(const SmartPlaylistTerm &term) {
  gtk_drop_down_set_selected(GTK_DROP_DOWN(field_), static_cast<guint>(term.field));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(op_), static_cast<guint>(term.op));
  gtk_editable_set_text(GTK_EDITABLE(value_), term.value.c_str());
}

bool SmartPlaylistSearchTermWidget::IsEmpty() const {
  const SmartPlaylistTerm term = Term();
  return term.value.empty() && term.op != SmartPlaylistOp::Empty && term.op != SmartPlaylistOp::NotEmpty;
}
