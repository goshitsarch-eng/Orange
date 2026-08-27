#ifndef STRAWBERRY_SONGLOADERINSERTERPLAN_H
#define STRAWBERRY_SONGLOADERINSERTERPLAN_H

namespace SongLoaderInserterPlan {

// Qt SongLoaderInserter::AsyncLoad task labels.
inline const char *PreloadTaskName() { return "Loading tracks"; }
inline const char *MetadataTaskName() { return "Loading tracks info"; }

// Qt AsyncLoad: full tags for the first successful pending loader so Play can start immediately.
inline bool ShouldLoadFirstMetadata(bool first_loaded, bool loader_ok) { return loader_ok && !first_loaded; }

inline int InsertAt(int row, int playlist_count) {
  if (row < 0 || row > playlist_count) {
    return playlist_count;
  }
  return row;
}

}  // namespace SongLoaderInserterPlan

#endif
