#ifndef STRAWBERRY_COLLECTIONCOMPILATIONDETECT_H
#define STRAWBERRY_COLLECTIONCOMPILATIONDETECT_H

namespace CollectionCompilationDetect {

inline bool IsCompilationAlbum(int distinct_artists) { return distinct_artists > 1; }

}  // namespace CollectionCompilationDetect

#endif  // STRAWBERRY_COLLECTIONCOMPILATIONDETECT_H
