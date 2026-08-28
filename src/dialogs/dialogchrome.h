#ifndef STRAWBERRY_DIALOGCHROME_H
#define STRAWBERRY_DIALOGCHROME_H

#include <adwaita.h>

// AdwDialog draws no titlebar of its own.  Handing it a bare content widget leaves the dialog with neither
// its title nor a close button on screen -- adw_dialog_set_title() then only reaches assistive technologies
// -- so every dialog gets an AdwHeaderBar inside an AdwToolbarView, which is what libadwaita expects.
namespace DialogChrome {

inline void SetContent(AdwDialog *dialog, GtkWidget *content) {
  GtkWidget *view = adw_toolbar_view_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(view), adw_header_bar_new());
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(view), content);
  adw_dialog_set_child(dialog, view);
}

}  // namespace DialogChrome

#endif
