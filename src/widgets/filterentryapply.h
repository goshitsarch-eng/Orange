#ifndef STRAWBERRY_FILTERENTRYAPPLY_H
#define STRAWBERRY_FILTERENTRYAPPLY_H

#include "collection/collectionfilterfocus.h"

#include <gtk/gtk.h>

#include <string>

namespace FilterEntryApply {

// Qt AutoExpandingTreeView FocusOnFilter: setFocus + sendEvent (Escape clears, Backspace deletes).
inline void FromKey(GtkWidget *entry, unsigned keyval) {
  if (!entry) {
    return;
  }
  gtk_widget_grab_focus(entry);
  const char *current = gtk_editable_get_text(GTK_EDITABLE(entry));
  const std::string next = CollectionFilterFocus::Apply(current ? current : "", CollectionFilterFocus::KeyEffect(keyval));
  if (next != (current ? current : "")) {
    gtk_editable_set_text(GTK_EDITABLE(entry), next.c_str());
    gtk_editable_set_position(GTK_EDITABLE(entry), -1);
  }
}

}  // namespace FilterEntryApply

#endif
