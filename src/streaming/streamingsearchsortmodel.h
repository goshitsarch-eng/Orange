#ifndef STRAWBERRY_STREAMINGSEARCHSORTMODEL_H
#define STRAWBERRY_STREAMINGSEARCHSORTMODEL_H

#include "streaming/streamingsearchmodel.h"

class StreamingSearchSortModel {
 public:
  explicit StreamingSearchSortModel(StreamingSearchModel *source);

  void SetSource(StreamingSearchModel *source);
  SongList Visible() const;
  void SetSort(StreamingSearchModel::SortField field, bool descending = false);

 private:
  StreamingSearchModel *source_ = nullptr;
};

#endif
