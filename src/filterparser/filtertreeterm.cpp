#include "filterparser/filtertreeterm.h"

FilterTreeTerm::FilterTreeTerm(std::unique_ptr<FilterParserSearchTermComparator> comparator, std::string sql)
    : cmp_(std::move(comparator)), sql_(std::move(sql)) {}

bool FilterTreeTerm::accept(const Song &song) const {
  if (!cmp_) {
    return true;
  }
  return cmp_->Matches(song.title()) || cmp_->Matches(song.album()) || cmp_->Matches(song.artist()) ||
         cmp_->Matches(song.albumartist()) || cmp_->Matches(song.composer()) || cmp_->Matches(song.performer()) ||
         cmp_->Matches(song.genre()) || cmp_->Matches(song.comment());
}
