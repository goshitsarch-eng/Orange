#ifndef STRAWBERRY_EDITTAGSHOW_H
#define STRAWBERRY_EDITTAGSHOW_H

namespace EditTagShow {

// Qt EditTagDialog::showEvent resizes to sizeHint().height() on programmatic show.
inline bool ShouldShrinkOnPresent(bool spontaneous = false) { return !spontaneous; }

// First open (and every GTK Show) uses content height unless the user stored geometry.
inline bool ShouldApplyDefaultHeight(bool has_stored_geometry) { return has_stored_geometry; }

}  // namespace EditTagShow

#endif
