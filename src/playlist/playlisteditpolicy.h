#ifndef STRAWBERRY_PLAYLISTEDITPOLICY_H
#define STRAWBERRY_PLAYLISTEDITPOLICY_H

namespace PlaylistEditPolicy {

// Qt MainWindow::EditValue always calls PlaylistView::edit(). The inline-edit setting only adds SelectedClicked.
inline bool MenuEditRequiresInlineSetting() { return false; }

inline bool SelectedClickStartsEdit(bool inline_enabled, bool already_selected) { return inline_enabled && already_selected; }

}  // namespace PlaylistEditPolicy

#endif
