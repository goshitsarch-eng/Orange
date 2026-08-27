#ifndef STRAWBERRY_SONGLOADERINSERTERPLAN_H
#define STRAWBERRY_SONGLOADERINSERTERPLAN_H

namespace SongLoaderInserterPlan {

// Qt SongLoaderInserter::AsyncLoad task labels.
inline const char *PreloadTaskName() { return "Loading tracks"; }
inline const char *MetadataTaskName() { return "Loading tracks info"; }

// Qt AsyncLoad: full tags for the first successful pending loader so Play can start immediately.
inline bool ShouldLoadFirstMetadata(bool first_loaded, bool loader_ok) { return loader_ok && !first_loaded; }

}  // namespace SongLoaderInserterPlan

#endif
