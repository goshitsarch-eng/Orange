#include "filterparser/filtertreenot.h"

FilterTreeNot::FilterTreeNot(std::unique_ptr<FilterTree> child) : child_(std::move(child)) {}

std::string FilterTreeNot::ToSql() const { return child_ ? "(NOT " + child_->ToSql() + ")" : "1=1"; }

bool FilterTreeNot::accept(const Song &song) const { return child_ ? !child_->accept(song) : true; }
