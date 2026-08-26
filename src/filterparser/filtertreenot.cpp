#include "filterparser/filtertreenot.h"

FilterTreeNot::FilterTreeNot(std::unique_ptr<FilterTree> child) : child_(std::move(child)) {}

bool FilterTreeNot::accept(const Song &song) const { return child_ ? !child_->accept(song) : true; }
