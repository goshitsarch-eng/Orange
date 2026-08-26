#include "utilities/filefilters.h"

#include "constants/filefilterconstants.h"
#include "translations/translations.h"

#include <initializer_list>
#include <string>

GtkFileFilter *FileFilters::FromGlobs(const char *name, const char *globs) {
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, name);
  for (const std::string &glob : FileFilterConstants::SplitGlobs(globs)) {
    gtk_file_filter_add_pattern(filter, glob.c_str());
#if GTK_CHECK_VERSION(4, 4, 0)
    if (glob.size() > 2 && glob[0] == '*' && glob[1] == '.') {
      gtk_file_filter_add_suffix(filter, glob.c_str() + 2);
    }
#endif
  }
  return filter;
}

namespace {

GListStore *Store(std::initializer_list<GtkFileFilter *> filters) {
  GListStore *store = g_list_store_new(GTK_TYPE_FILE_FILTER);
  for (GtkFileFilter *filter : filters) {
    g_list_store_append(store, filter);
    g_object_unref(filter);
  }
  return store;
}

GtkFileFilter *AllFiles() { return FileFilters::FromGlobs(Translations::CStr("All Files"), FileFilterConstants::kAllFiles); }

}  // namespace

GListStore *FileFilters::MediaFilters() {
  return Store({FromGlobs(Translations::CStr("Media files"), FileFilterConstants::kFileFilter), AllFiles()});
}

GListStore *FileFilters::AudioFilters() {
  return Store({FromGlobs(Translations::CStr("Audio files"), FileFilterConstants::kAudio), AllFiles()});
}

GListStore *FileFilters::PlaylistFilters() {
  return Store({FromGlobs(Translations::CStr("Playlists"), FileFilterConstants::kPlaylist), AllFiles()});
}

GListStore *FileFilters::ImageFilters(bool save) {
  return Store({FromGlobs(Translations::CStr("Images"), save ? FileFilterConstants::kSaveImages : FileFilterConstants::kLoadImages), AllFiles()});
}

void FileFilters::Apply(GtkFileDialog *dialog, GListStore *filters) {
  gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
  if (g_list_model_get_n_items(G_LIST_MODEL(filters)) > 0) {
    gpointer first = g_list_model_get_item(G_LIST_MODEL(filters), 0);
    gtk_file_dialog_set_default_filter(dialog, GTK_FILE_FILTER(first));
    g_object_unref(first);
  }
  g_object_unref(filters);
}
