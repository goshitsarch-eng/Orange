#ifndef STRAWBERRY_STREAMINGSEARCHMODEL_H
#define STRAWBERRY_STREAMINGSEARCHMODEL_H

#include "core/song.h"
#include "streaming/streamingservices.h"

#include <string>

class StreamingSearchModel {
 public:
  enum class SortField {
    Title,
    Artist,
    Album
  };

  void SetSongs(const SongList &songs);
  void SetFilter(const std::string &filter);
  void SetSort(SortField field, bool descending = false);
  void SetSearchType(StreamingService::SearchType type);

  const SongList &songs() const { return songs_; }
  SongList Visible() const;
  const std::string &filter() const { return filter_; }
  SortField sort_field() const { return sort_field_; }
  bool sort_descending() const { return descending_; }
  StreamingService::SearchType search_type() const { return search_type_; }

  static int Compare(const Song &left, const Song &right, SortField field);

 private:
  SongList songs_;
  std::string filter_;
  SortField sort_field_ = SortField::Title;
  bool descending_ = false;
  StreamingService::SearchType search_type_ = StreamingService::SearchType::Songs;
};

#endif
