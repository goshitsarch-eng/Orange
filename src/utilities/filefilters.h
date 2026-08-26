#ifndef STRAWBERRY_FILEFILTERS_H
#define STRAWBERRY_FILEFILTERS_H

#include <gtk/gtk.h>

namespace FileFilters {

GtkFileFilter *FromGlobs(const char *name, const char *globs);
GListStore *MediaFilters();
GListStore *AudioFilters();
GListStore *PlaylistFilters();
GListStore *ImageFilters(bool save);
void Apply(GtkFileDialog *dialog, GListStore *filters);

}  // namespace FileFilters

#endif
