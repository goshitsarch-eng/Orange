#include "streaming/streamingsearchsortmodel.h"

StreamingSearchSortModel::StreamingSearchSortModel(StreamingSearchModel *source) : source_(source) {}

void StreamingSearchSortModel::SetSource(StreamingSearchModel *source) { source_ = source; }

SongList StreamingSearchSortModel::Visible() const { return source_ ? source_->Visible() : SongList{}; }

void StreamingSearchSortModel::SetSort(StreamingSearchModel::SortField field, bool descending) {
  if (source_) {
    source_->SetSort(field, descending);
  }
}
